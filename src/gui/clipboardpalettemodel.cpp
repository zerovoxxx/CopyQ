// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardpalettemodel.h"

#include "common/appconfig.h"
#include "common/config.h"
#include "common/contenttype.h"
#include "common/mimetypes.h"
#include "common/textdata.h"
#include "gui/filterlineedit.h"
#include "item/itemfactory.h"

#include <QElapsedTimer>
#include <QBuffer>
#include <QImageReader>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>
#include <QUrl>
#include <algorithm>

namespace {
QString itemType(const QVariantMap &data)
{
    if (data.contains(mimeUriList))
        return QStringLiteral("Files / links");
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        if (it.key().startsWith(QLatin1String("image/")))
            return QStringLiteral("Image");
    }
    return data.contains(mimeHtml) ? QStringLiteral("HTML") : QStringLiteral("Text");
}
}

PaletteImageProvider::PaletteImageProvider(ClipboardPaletteModel *model)
    : QQuickImageProvider(QQuickImageProvider::Image), m_model(model)
{}

QImage PaletteImageProvider::requestImage(const QString &, QSize *size, const QSize &requestedSize)
{
    QImage image;
    const auto data = m_model ? m_model->displayData() : QVariantMap();
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        if (!it.key().startsWith(QLatin1String("image/")))
            continue;
        auto bytes = it.value().toByteArray();
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer);
        const auto original = reader.size();
        const auto limit = requestedSize.isValid() ? requestedSize : QSize(720, 720);
        if (original.isValid())
            reader.setScaledSize(original.scaled(limit, Qt::KeepAspectRatio));
        image = reader.read();
        break;
    }
    *size = image.size();
    return image;
}

ClipboardPaletteModel::ClipboardPaletteModel(ItemFactory *factory, QObject *parent)
    : QAbstractListModel(parent), m_factory(factory)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &ClipboardPaletteModel::filterBatch);
}

void ClipboardPaletteModel::setSourceModel(QAbstractItemModel *source, const QString &tabName)
{
    if (m_source == source && m_tabName == tabName)
        return;
    for (const auto &connection : m_sourceConnections)
        disconnect(connection);
    m_sourceConnections.clear();
    m_source = source;
    m_tabName = tabName;
    m_selected = QPersistentModelIndex();
    m_anchor = QPersistentModelIndex();
    m_selection.clear();
    emit selectionInvalidated();
    if (source) {
        const auto changed = [this]() { rebuild(false); };
        m_sourceConnections = {
            connect(source, &QAbstractItemModel::rowsInserted, this, changed),
            connect(source, &QAbstractItemModel::rowsRemoved, this, changed),
            connect(source, &QAbstractItemModel::rowsMoved, this, changed),
            connect(source, &QAbstractItemModel::layoutChanged, this, changed),
            connect(source, &QAbstractItemModel::dataChanged, this, changed),
            connect(source, &QAbstractItemModel::modelReset, this, changed),
            connect(source, &QObject::destroyed, this, [this]() {
                m_source = nullptr;
                rebuild(false);
            })
        };
    }
    rebuild(true);
}

int ClipboardPaletteModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_results.size();
}

int ClipboardPaletteModel::sourceCount() const
{
    return m_source ? m_source->rowCount() : 0;
}

QHash<int, QByteArray> ClipboardPaletteModel::roleNames() const
{
    return {{SummaryRole, "summary"}, {TypeRole, "itemType"}, {SelectedRole, "itemSelected"},
        {NotesRole, "notes"}, {TagsRole, "tags"}, {PinnedRole, "pinned"}};
}

QVariant ClipboardPaletteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return QVariant();
    const auto source = m_results.at(index.row());
    if (!source.isValid())
        return QVariant();
    const auto data = source == m_selected ? m_displayData : source.data(contentType::data).toMap();
    if (role == SelectedRole)
        return m_selection.contains(source);
    if (role == NotesRole)
        return getTextData(data, mimeItemNotes).left(512);
    if (role == TagsRole)
        return QString::fromUtf8(data.value(QStringLiteral(COPYQ_MIME_PREFIX "tags")).toByteArray());
    if (role == PinnedRole)
        return data.contains(QStringLiteral(COPYQ_MIME_PREFIX "item-pinned"));
    if (role == TypeRole)
        return itemType(data);
    if (role == SummaryRole) {
        if (source.data(contentType::isHidden).toBool())
            return tr("Hidden item");
        auto text = getTextData(data).left(512).simplified();
        if (text.isEmpty())
            text = QString::fromUtf8(data.value(mimeUriList).toByteArray()).left(512).simplified();
        return text.isEmpty() ? itemType(data) : text;
    }
    return QVariant();
}

void ClipboardPaletteModel::setQuery(const QString &query)
{
    if (m_query == query)
        return;
    m_query = query;
    emit queryChanged();
    rebuild(true);
}

void ClipboardPaletteModel::rebuild(bool selectFirst)
{
    m_timer.stop();
    if (!m_selected.isValid())
        emit selectionInvalidated();
    const AppConfig config;
    m_filter = Utils::FilterLineEdit::createFilter(m_query,
        config.option<Config::filter_regular_expression>(),
        config.option<Config::filter_case_insensitive>() ? Qt::CaseInsensitive : Qt::CaseSensitive);
    m_selectFirst = selectFirst;
    if (selectFirst) {
        m_selected = QPersistentModelIndex();
        m_anchor = QPersistentModelIndex();
        m_selection.clear();
    }
    beginResetModel();
    m_results.clear();
    m_nextRow = 0;
    endResetModel();
    updatePreview();
    m_timer.start(0);
    emit resultsChanged();
    emit selectionChanged();
}

void ClipboardPaletteModel::filterBatch()
{
    // Plugin matching stays on the model's GUI thread. Restarting the timer discards old work.
    QElapsedTimer elapsed;
    elapsed.start();
    QList<QPersistentModelIndex> matches;
    while (m_nextRow < sourceCount() && elapsed.elapsed() < 5) {
        const auto index = m_source->index(m_nextRow++, 0);
        if (m_filter->matchesAll() || (!m_filter->matchesNone()
                && m_factory && m_factory->matches(index, *m_filter)))
            matches.append(index);
    }
    if (!matches.isEmpty()) {
        const int first = m_results.size();
        beginInsertRows(QModelIndex(), first, first + matches.size() - 1);
        m_results.append(matches);
        endInsertRows();
    }
    if (m_nextRow < sourceCount()) {
        m_timer.start(0);
    } else {
        if (m_selectFirst && !m_results.isEmpty()) {
            m_selected = m_results.first();
            m_anchor = m_selected;
            m_selection = {m_selected};
        } else if (!m_results.contains(m_selected)) {
            m_selected = QPersistentModelIndex();
            emit selectionInvalidated();
        }
        const QSet<QPersistentModelIndex> results(m_results.cbegin(), m_results.cend());
        for (auto it = m_selection.begin(); it != m_selection.end();) {
            if (!it->isValid() || !results.contains(*it))
                it = m_selection.erase(it);
            else
                ++it;
        }
        m_selectFirst = false;
        if (!m_results.isEmpty())
            emit dataChanged(index(0), index(count() - 1), {SelectedRole});
        updatePreview();
        emit selectionChanged();
    }
    emit resultsChanged();
}

int ClipboardPaletteModel::selectedRow() const
{
    return m_selected.isValid() ? m_results.indexOf(m_selected) : -1;
}

void ClipboardPaletteModel::selectRow(int row)
{
    select(row);
}

QPersistentModelIndex ClipboardPaletteModel::sourceIndex(int row) const
{
    return row >= 0 && row < m_results.size() ? m_results.at(row) : QPersistentModelIndex();
}

int ClipboardPaletteModel::resultRow(const QModelIndex &index) const
{
    return index.isValid() ? m_results.indexOf(index) : -1;
}

bool ClipboardPaletteModel::isSelected(int row) const
{
    const auto index = sourceIndex(row);
    return index.isValid() && m_selection.contains(index);
}

QList<QPersistentModelIndex> ClipboardPaletteModel::selectedIndexes() const
{
    auto indexes = m_selection;
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

void ClipboardPaletteModel::setSelectedIndexes(const QList<QPersistentModelIndex> &indexes)
{
    if (filtering())
        return;
    m_selection.clear();
    const QSet<QPersistentModelIndex> results(m_results.cbegin(), m_results.cend());
    QSet<QPersistentModelIndex> selected;
    for (const auto &index : indexes) {
        if (index.isValid() && results.contains(index) && !selected.contains(index)) {
            m_selection.append(index);
            selected.insert(index);
        }
    }
    m_selected = m_selection.isEmpty() ? QPersistentModelIndex() : m_selection.last();
    m_anchor = m_selected;
    if (!m_results.isEmpty())
        emit dataChanged(this->index(0), this->index(count() - 1), {SelectedRole});
    updatePreview();
    emit selectionChanged();
}

void ClipboardPaletteModel::select(int row, bool extend, bool toggle)
{
    if (filtering())
        return;
    const auto index = sourceIndex(row);
    if (!extend && !toggle && m_selected == index
            && m_selection == (index.isValid() ? QList<QPersistentModelIndex>{index} : QList<QPersistentModelIndex>()))
        return;
    if (extend && m_results.contains(m_anchor) && index.isValid()) {
        const int anchor = m_results.indexOf(m_anchor);
        if (!toggle)
            m_selection.clear();
        QSet<QPersistentModelIndex> selected(m_selection.cbegin(), m_selection.cend());
        for (int i = qMin(anchor, row); i <= qMax(anchor, row); ++i) {
            if (!selected.contains(m_results.at(i))) {
                m_selection.append(m_results.at(i));
                selected.insert(m_results.at(i));
            }
        }
    } else if (toggle && index.isValid()) {
        if (!m_selection.removeOne(index))
            m_selection.append(index);
        m_anchor = index;
    } else {
        m_selection = index.isValid() ? QList<QPersistentModelIndex>{index} : QList<QPersistentModelIndex>();
        m_anchor = index;
    }
    m_selected = index;
    if (!m_results.isEmpty())
        emit dataChanged(this->index(0), this->index(count() - 1), {SelectedRole});
    updatePreview();
    emit selectionChanged();
}

void ClipboardPaletteModel::selectAll()
{
    setSelectedIndexes(m_results);
}

void ClipboardPaletteModel::selectNext(int step)
{
    if (!m_results.isEmpty())
        selectRow(qBound(0, selectedRow() + step, m_results.size() - 1));
}

QVariantMap ClipboardPaletteModel::selectedData() const
{
    return m_selected.isValid() ? m_selected.data(contentType::data).toMap() : QVariantMap();
}

QVariantMap ClipboardPaletteModel::displayData() const
{
    return m_displayData;
}

void ClipboardPaletteModel::updatePreview()
{
    ++m_displayRevision;
    ++m_previewRevision;
    m_displayData = selectedData();
    emit previewChanged();
    if (m_selected.isValid()) {
        auto data = m_displayData;
        // Display scripts must never resolve the old management window's selection.
        data.insert(mimeSelectedItems, QByteArray());
        emit itemDisplayRequested(PersistentDisplayItem(this, m_selected, m_displayRevision, data));
    }
}

void ClipboardPaletteModel::setDisplayData(
        const QPersistentModelIndex &index, int revision, const QVariantMap &data)
{
    if (revision != m_displayRevision || index != m_selected || !index.isValid() || data.isEmpty())
        return;
    m_displayData = data;
    ++m_previewRevision;
    if (selectedRow() >= 0)
        emit dataChanged(this->index(selectedRow()), this->index(selectedRow()));
    emit previewChanged();
}

QVariantMap ClipboardPaletteModel::preview() const
{
    if (!m_selected.isValid())
        return QVariantMap();
    if (m_selected.data(contentType::isHidden).toBool())
        return {{QStringLiteral("text"), tr("Hidden item")}};
    QVariantMap result{
        {QStringLiteral("text"), getTextData(m_displayData).left(100000)},
        {QStringLiteral("html"), QString::fromUtf8(m_displayData.value(mimeHtml).toByteArray()).left(100000)},
        {QStringLiteral("type"), itemType(m_displayData)},
        {QStringLiteral("editable"), selectedData().contains(mimeText) || selectedData().contains(mimeHtml)},
        {QStringLiteral("image"), QStringLiteral("image://palette/%1").arg(m_previewRevision)}
    };
    result.insert(QStringLiteral("notes"), getTextData(m_displayData, mimeItemNotes));
    result.insert(QStringLiteral("tags"), QString::fromUtf8(m_displayData.value(QStringLiteral(COPYQ_MIME_PREFIX "tags")).toByteArray()));
    result.insert(QStringLiteral("pinned"), m_displayData.contains(QStringLiteral(COPYQ_MIME_PREFIX "item-pinned")));
    QStringList urls;
    const auto lines = QString::fromUtf8(m_displayData.value(mimeUriList).toByteArray()).split('\n');
    for (const auto &line : lines) {
        if (!line.startsWith('#') && !line.trimmed().isEmpty())
            urls.append(line.trimmed());
    }
    if (urls.isEmpty()) {
        const QUrl url(result.value(QStringLiteral("text")).toString().trimmed());
        if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
            urls.append(url.toString());
    }
    const auto html = result.value(QStringLiteral("html")).toString();
    if (!html.isEmpty()) {
        QTextDocument document;
        document.setHtml(html);
        for (auto block = document.begin(); block.isValid(); block = block.next()) {
            for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment) {
                const auto href = fragment.fragment().charFormat().anchorHref();
                const QUrl url(href);
                if ((url.scheme() == QLatin1String("file") || url.scheme() == QLatin1String("http")
                        || url.scheme() == QLatin1String("https")) && !urls.contains(href))
                    urls.append(href);
            }
        }
    }
    if (!urls.isEmpty() && QUrl(urls.first()).isLocalFile())
        result.insert(QStringLiteral("filePath"), QUrl(urls.first()).toLocalFile());
    result.insert(QStringLiteral("urls"), urls);
    return result;
}
