// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardsnippets.h"
#include "item/snippetstore.h"
#include "item/itemeditorwidget.h"
#include "common/mimetypes.h"
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QQuickItem>
#include <QSaveFile>
#include <QScreen>
#include <QVBoxLayout>

ClipboardSnippets::ClipboardSnippets(SnippetStore *store, const ClipboardBrowserSharedPtr &shared)
    : m_store(store), m_shared(shared)
{
    setObjectName(QStringLiteral("clipboard_snippets"));
    setTitle(tr("QClip — Snippets"));
    setMinimumSize(QSize(720, 480));
    setResizeMode(QQuickView::SizeRootObjectToView);
    connect(m_store, &SnippetStore::changed, this, &ClipboardSnippets::changed);
}
ClipboardSnippets::~ClipboardSnippets() { QQuickView::setSource(QUrl()); }
void ClipboardSnippets::open(const PlatformWindowPtr &target)
{
    m_target = target;
    if (status() != QQuickView::Ready) {
        setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(this)}});
        QQuickView::setSource(QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/Snippets.qml")));
    }
    if (status() != QQuickView::Ready) { m_error = tr("Unable to load Snippets."); emit changed(); return; }
    if (!isVisible() && screen()) {
        const auto area = screen()->availableGeometry();
        const QSize size(qMin(1040, area.width()), qMin(720, area.height()));
        setGeometry(QRect(area.center() - QPoint(size.width()/2, size.height()/2), size));
    }
    show(); raise(); requestActivate(); rootObject()->forceActiveFocus();
}
QVariantList ClipboardSnippets::collections() const { return m_store->collections(); }
QVariantMap ClipboardSnippets::selected() const
{
    auto value = m_store->snippet(m_selected);
    value.insert(QStringLiteral("resolvedKeyword"), m_store->effectiveKeyword(value));
    value.insert(QStringLiteral("conflict"), m_store->keywordError(m_selected));
    return value;
}
QVariantMap ClipboardSnippets::settings() const { return m_store->settings(); }
QVariantMap ClipboardSnippets::theme() const { return m_shared->theme.quickTheme(); }
QVariantList ClipboardSnippets::snippets() const
{
    QVariantList list;
    for (const auto &value : m_store->snippets()) {
        auto snippet = value.toMap();
        const auto keyword = m_store->effectiveKeyword(snippet);
        if (!m_collection.isEmpty() && snippet.value(QStringLiteral("collection")).toString() != m_collection) continue;
        if (!m_query.isEmpty() && !snippet.value(QStringLiteral("title")).toString().contains(m_query, Qt::CaseInsensitive)
                && !keyword.contains(m_query, Qt::CaseInsensitive)) continue;
        snippet.insert(QStringLiteral("resolvedKeyword"), keyword);
        snippet.insert(QStringLiteral("conflict"), m_store->keywordError(snippet.value(QStringLiteral("id")).toString()));
        list.append(snippet);
    }
    return list;
}
void ClipboardSnippets::select(const QString &id) { m_selected = id; m_error.clear(); emit changed(); }
bool ClipboardSnippets::commit(const QVariantMap &before, bool success)
{
    success = success && (!m_shared->tabsEncrypted || m_shared->encryptionKey.isValid()) && m_store->save(m_shared->encryptionKey);
    m_error = success ? QString() : m_store->error();
    if (!success) {
        if (m_error.isEmpty()) m_error = tr("Unlock encrypted storage before saving.");
        m_store->setDocument(before);
    }
    emit changed();
    return success;
}
QString ClipboardSnippets::createCollection(const QString &name)
{
    const auto before = m_store->document();
    const auto id = m_store->createCollection(name);
    if (!commit(before, !id.isEmpty())) return {};
    m_collection = id; emit changed(); return id;
}
bool ClipboardSnippets::editCollection(const QString &id, const QVariantMap &fields)
{
    const auto before = m_store->document(); return commit(before, m_store->updateCollection(id, fields));
}
bool ClipboardSnippets::removeCollection(const QString &id)
{
    const auto before = m_store->document();
    if (!commit(before, m_store->removeCollection(id))) return false;
    if (m_collection == id) { m_collection.clear(); m_selected.clear(); emit changed(); }
    return true;
}
QString ClipboardSnippets::createSnippet()
{
    if (m_collection.isEmpty()) { m_error = tr("Select or create a collection first."); emit changed(); return {}; }
    const auto before = m_store->document();
    const auto id = m_store->createSnippet(m_collection, {{mimeText, QByteArray()}});
    if (!commit(before, !id.isEmpty())) return {};
    select(id); return id;
}
bool ClipboardSnippets::editSnippet(const QVariantMap &fields)
{
    const auto before = m_store->document();
    if (!commit(before, m_store->updateSnippet(m_selected, fields))) return false;
    emit snippetSaved();
    return true;
}
bool ClipboardSnippets::removeSnippet()
{
    const auto before = m_store->document();
    if (!commit(before, m_store->removeSnippet(m_selected))) return false;
    m_selected.clear(); emit changed(); return true;
}
SnippetResult ClipboardSnippets::render() const
{
    if (!m_store->writable() || (m_shared->tabsEncrypted && !m_shared->encryptionKey.isValid()))
        return {{}, tr("Unlock encrypted Snippet storage before use."), -1};
    const auto data = m_store->snippet(m_selected).value(QStringLiteral("data")).toMap();
    if (data.isEmpty()) return {{}, tr("Select a Snippet."), -1};
    return renderSnippet(data, m_context ? m_context() : SnippetContext());
}
QString ClipboardSnippets::preview() const
{
    const auto result = render();
    return result.valid() ? QString::fromUtf8(result.data.value(mimeText).toByteArray()) : result.error;
}
QString ClipboardSnippets::richPreview() const
{
    const auto result = render();
    return result.valid() ? QString::fromUtf8(result.data.value(mimeHtml).toByteArray()) : QString();
}
void ClipboardSnippets::useSnippet(bool paste)
{
    const auto result = render();
    m_error = result.error;
    if (result.valid()) emit outputRequested(result.data, result.cursor, paste, m_target);
    emit changed();
}
void ClipboardSnippets::runCommand()
{
    const auto result = render();
    m_error = result.error;
    const auto name = selected().value(QStringLiteral("command")).toString();
    if (result.valid() && !name.isEmpty()) emit commandRequested(name, result.data, m_target);
    emit changed();
}
void ClipboardSnippets::setSetting(const QString &name, const QVariant &value)
{
    const auto before = m_store->document();
    auto settings = m_store->settings(); settings.insert(name, value);
    m_store->setSettings(settings); commit(before, true);
}
void ClipboardSnippets::editBody()
{
    const auto snippet = m_store->snippet(m_selected);
    if (snippet.isEmpty()) return;
    const auto id = m_selected;
    auto dialog = new QDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("QClip — Edit Snippet"));
    dialog->resize(660, 440);
    auto layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(24, 20, 24, 20);
    auto editor = new ItemEditorWidget({}, mimeText, dialog);
    const auto data = snippet.value(QStringLiteral("data")).toMap();
    if (data.contains(mimeHtml)) editor->setHtml(QString::fromUtf8(data.value(mimeHtml).toByteArray()));
    else editor->setPlainText(QString::fromUtf8(data.value(mimeText).toByteArray()));
    editor->setFont(m_shared->theme.editorFont()); editor->setPalette(m_shared->theme.editorPalette());
    layout->addWidget(editor->createToolbar(dialog, m_shared->menuItems)); layout->addWidget(editor);
    auto error = new QLabel(dialog); error->setWordWrap(true); layout->addWidget(error);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog); layout->addWidget(buttons);
    const auto save = [this, id, data, editor, error, dialog] {
        auto updated = data;
        updated.remove(mimeHtml);
        const auto edited = editor->data();
        for (auto it = edited.cbegin(); it != edited.cend(); ++it) updated.insert(it.key(), it.value());
        const auto result = renderSnippet(updated, m_context ? m_context() : SnippetContext());
        if (!result.valid()) { error->setText(result.error); return; }
        const auto before = m_store->document();
        if (commit(before, m_store->updateSnippet(id, {{QStringLiteral("data"), updated}}))) dialog->accept();
        else error->setText(m_error);
    };
    connect(buttons, &QDialogButtonBox::accepted, this, save);
    connect(editor, &ItemEditorWidget::save, this, save);
    connect(editor, &ItemEditorWidget::cancel, dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(this, &QObject::destroyed, dialog, &QWidget::close);
    m_shared->theme.decorateMainWindow(dialog);
    dialog->show(); editor->setFocus();
}
void ClipboardSnippets::importFile()
{
    const auto path = QFileDialog::getOpenFileName(nullptr, tr("Import QClip collection"), {}, tr("QClip Snippets (*.qcs)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 100'000'000) { m_error = tr("Unable to read collection."); emit changed(); return; }
    const auto before = m_store->document(); commit(before, m_store->importCollection(file.readAll()));
}
void ClipboardSnippets::exportFile()
{
    const auto bytes = m_store->exportCollection(m_collection);
    if (bytes.isEmpty()) return;
    const auto path = QFileDialog::getSaveFileName(nullptr, tr("Export QClip collection"), {}, tr("QClip Snippets (*.qcs)"));
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) m_error = file.errorString();
    else m_error.clear();
    emit changed();
}
void ClipboardSnippets::importCopyQ()
{
    const auto directory = QFileDialog::getExistingDirectory(nullptr, tr("Select isolated CopyQ profile"));
    if (!directory.isEmpty()) emit copyQImportRequested(directory);
}
