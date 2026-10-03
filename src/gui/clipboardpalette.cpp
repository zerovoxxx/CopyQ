// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardpalette.h"

#include "common/mimetypes.h"
#include "common/log.h"
#include "platform/platformwindow.h"

#include <QAction>
#include <QCursor>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QFileInfo>
#include <QImageReader>
#include <QInputMethod>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QScreen>
#include <QShortcutEvent>
#include <QBuffer>

Q_IMPORT_QML_PLUGIN(QClipPlugin)

ClipboardPalette::ClipboardPalette(ItemFactory *factory)
    : m_model(factory, this)
{
    setObjectName(QStringLiteral("clipboard_palette"));
    setTitle(QStringLiteral("QClip — Clipboard Palette"));
    setFlags(Qt::Tool | Qt::FramelessWindowHint);
    setResizeMode(QQuickView::SizeRootObjectToView);
    m_model.setDisplayEnabled(false);
    connect(this, &QWindow::visibleChanged, this, [this] { m_model.setDisplayEnabled(isVisible()); });
    engine()->addImageProvider(QStringLiteral("palette"), new PaletteImageProvider(&m_model));
    connect(&m_model, &ClipboardPaletteModel::selectionInvalidated, this, [this]() {
        if (m_busy && !pendingValid()) {
            COPYQ_LOG("Palette selection became invalid");
            cancel();
        }
    });
    connect(this, &QWindow::activeChanged, this, [this]() {
        if (isActive())
            m_wasActive = true;
        else if (isVisible() && m_wasActive) {
            QTimer::singleShot(0, this, [this]() {
                auto focus = QGuiApplication::focusWindow();
                for (auto window = focus; window; window = window->transientParent()) {
                    if (window == this)
                        return;
                }
                if (isVisible() && !isActive()) {
                    const auto native = platformNativeInterface()->getWindow(winId());
                    const auto current = platformNativeInterface()->getCurrentWindow();
                    COPYQ_LOG(QStringLiteral("Palette focus lost: Qt focus %1, native active %2, frontmost application %3")
                        .arg(focus ? focus->objectName() : QStringLiteral("<null>"))
                        .arg(native && native->isActive())
                        .arg(current ? current->getApplicationId() : QStringLiteral("<null>")));
                    cancel();
                }
            });
        }
    });
}

ClipboardPalette::~ClipboardPalette()
{
    disconnect(this, nullptr, this, nullptr);
    QQuickView::setSource(QUrl());
    hide();
}

bool ClipboardPalette::load()
{
    if (status() == QQuickView::Ready)
        return true;
    setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(this)}});
    setSource(QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/ClipboardPalette.qml")));
    if (status() != QQuickView::Ready) {
        QStringList messages;
        for (const auto &error : errors())
            messages.append(error.toString());
        setError(tr("Unable to load clipboard palette: %1").arg(messages.join('\n')));
        return false;
    }
    return true;
}

void ClipboardPalette::open(QAbstractItemModel *source, const QString &tabName,
        const QStringList &tabs, const PlatformWindowPtr &target)
{
    cancel();
    if (!load())
        return;
    m_target = target;
    m_tabs = tabs;
    m_error.clear();
    m_wasActive = false;
    m_model.setSourceModel(source, tabName);
    m_model.setQuery(QString());
    // A second invocation must still select the first result when the query was already empty.
    m_model.selectRow(0);
    emit sourceChanged();
    emit stateChanged();
    auto screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (screen) {
        const auto area = screen->availableGeometry();
        const auto size = QSize(qMin(740, area.width()), qMin(440, area.height()));
        setGeometry(QRect(area.center() - QPoint(size.width() / 2, size.height() / 2), size));
    }
    show();
    raise();
    requestActivate();
    if (auto window = platformNativeInterface()->getWindow(winId()))
        window->raise();
    emit opened();
}

void ClipboardPalette::setHistorySource(QAbstractItemModel *source, const QString &tabName)
{
    if (m_busy)
        cancel();
    m_model.setSourceModel(source, tabName);
    emit sourceChanged();
}

bool ClipboardPalette::pendingValid() const
{
    return m_busy && m_pendingIndex.isValid() && m_pendingIndex.model() == m_model.sourceModel();
}

void ClipboardPalette::activate(bool paste, bool plainText, bool bypassDefault)
{
    if (m_busy || m_model.filtering() || composing() || !m_model.selectedIndex().isValid())
        return;
    if (paste && !plainText && !bypassDefault && m_commands && m_commands->defaultAction()) {
        auto action = m_commands->defaultAction();
        if (action->isEnabled()) {
            action->trigger();
        } else {
            setError(tr("The Enter command is unavailable. Use Alt+Enter to paste."));
        }
        return;
    }
    if (paste && (!m_target || !m_target->isValid())) {
        setError(tr("The original window is unavailable. You can still copy this item."));
        return;
    }
    auto data = m_model.selectedData();
    if (plainText) {
        if (!data.contains(mimeText)) {
            setError(tr("This item has no plain text."));
            return;
        }
        data = {{mimeText, data.value(mimeText)}};
    }
    if (data.isEmpty())
        return;
    m_pendingIndex = m_model.selectedIndex();
    m_pendingData = data;
    ++m_activationId;
    m_busy = true;
    m_error.clear();
    emit stateChanged();
    emit activationRequested(m_pendingIndex, data, paste);
}

void ClipboardPalette::cancel()
{
    const bool wasBusy = m_busy;
    if (wasBusy)
        COPYQ_LOG("Cancelling pending palette activation");
    m_busy = false;
    m_pendingIndex = QPersistentModelIndex();
    m_pendingData.clear();
    ++m_activationId;
    hide();
    if (wasBusy)
        emit activationCancelled();
    emit stateChanged();
}

void ClipboardPalette::complete(const QString &error)
{
    m_busy = false;
    m_pendingIndex = QPersistentModelIndex();
    m_pendingData.clear();
    if (error.isEmpty())
        hide();
    setError(error);
}

bool ClipboardPalette::beginCommand()
{
    if (m_busy || m_model.filtering() || !m_model.selectedIndex().isValid())
        return false;
    m_pendingIndex = m_model.selectedIndex();
    m_pendingData = m_model.selectedData();
    m_busy = true;
    ++m_activationId;
    m_error.clear();
    emit stateChanged();
    return true;
}

void ClipboardPalette::setError(const QString &error)
{
    if (!error.isEmpty())
        COPYQ_LOG(QStringLiteral("Palette: %1").arg(error));
    m_error = error;
    emit stateChanged();
    if (!error.isEmpty())
        emit errorOccurred(error);
}

bool ClipboardPalette::composing() const
{
    auto search = rootObject() ? rootObject()->findChild<QObject*>(QStringLiteral("palette_search")) : nullptr;
    return search && search->property("inputMethodComposing").toBool();
}

bool ClipboardPalette::event(QEvent *event)
{
    if (event->type() == QEvent::Close) {
        cancel();
        event->ignore();
        return true;
    }
    if (event->type() == QEvent::KeyPress) {
        if (rootObject() && (rootObject()->property("popupActive").toBool()
                || !rootObject()->property("historyFocus").toBool()))
            return ClipboardWindow::event(event);
        auto key = static_cast<QKeyEvent*>(event);
        // Let TextField and the input method consume composition keys first.
        if (!composing()) {
            if (key->key() == Qt::Key_Escape) {
                cancel();
                return true;
            }
            if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up) {
                m_model.selectNext(key->key() == Qt::Key_Down ? 1 : -1);
                return true;
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                if (!key->isAutoRepeat()) {
                    const auto mods = key->modifiers();
                    activate(!mods.testFlag(Qt::ControlModifier), mods.testFlag(Qt::ShiftModifier),
                             mods.testFlag(Qt::AltModifier));
                }
                return true;
            }
            if (m_commands && !m_busy) {
                const QKeySequence sequence(key->keyCombination());
                for (auto action : m_commands->actions()) {
                    if (action->isEnabled() && action->shortcuts().contains(sequence)) {
                        QShortcutEvent shortcut(sequence, 0);
                        QCoreApplication::sendEvent(action, &shortcut);
                        return true;
                    }
                }
            }
        }
    }
    return ClipboardWindow::event(event);
}

void ClipboardPalette::changeSource(const QString &tabName)
{
    if (!m_busy && m_tabs.contains(tabName))
        emit sourceRequested(tabName);
}

void ClipboardPalette::openUrl(const QString &url)
{
    if (m_busy || !m_model.preview().value(QStringLiteral("urls")).toStringList().contains(url))
        return;
    const QUrl target(url);
    if (target.isLocalFile() && !QFileInfo::exists(target.toLocalFile())) {
        setError(tr("The selected file no longer exists."));
        return;
    }
    if (target.scheme() != QLatin1String("file") && target.scheme() != QLatin1String("http")
            && target.scheme() != QLatin1String("https")) {
        setError(tr("This link type cannot be opened."));
        return;
    }
    if (!QDesktopServices::openUrl(target))
        setError(tr("Unable to open the selected link."));
}

void ClipboardPalette::previewFile()
{
    const auto urls = m_model.preview().value(QStringLiteral("urls")).toStringList();
    if (urls.isEmpty())
        return;
    const QUrl url(urls.first());
    if (!url.isLocalFile() || !QFileInfo::exists(url.toLocalFile())) {
        setError(tr("The selected file is unavailable."));
        return;
    }
#ifdef Q_OS_MACOS
    if (!QProcess::startDetached(QStringLiteral("/usr/bin/qlmanage"), {QStringLiteral("-p"), url.toLocalFile()}))
        setError(tr("Unable to start Quick Look."));
#else
    openUrl(url.toString());
#endif
}

void ClipboardPalette::editItem()
{
    if (!m_busy && m_model.selectedIndex().isValid())
        emit editorRequested(m_model.selectedIndex());
}

void ClipboardPalette::showPluginSettings()
{
    if (!m_busy)
        emit pluginSettingsRequested();
}

void ClipboardPalette::setCommandMenu(QMenu *menu)
{
    m_commands = menu;
    emit commandsChanged();
}

QVariantList ClipboardPalette::commands() const
{
    QVariantList result;
    if (m_commands) {
        for (auto action : m_commands->actions()) {
            result.append(QVariantMap{
                {QStringLiteral("name"), action->text().remove('&')},
                {QStringLiteral("enabled"), action->isEnabled()},
                {QStringLiteral("shortcut"), action->shortcut().toString(QKeySequence::NativeText)}
            });
        }
    }
    return result;
}

QString ClipboardPalette::enterLabel() const
{
    if (m_commands && m_commands->defaultAction())
        return m_commands->defaultAction()->text().remove('&');
    return tr("Paste");
}

void ClipboardPalette::triggerCommand(int index)
{
    if (!m_busy && m_commands && index >= 0 && index < m_commands->actions().size()) {
        auto action = m_commands->actions().at(index);
        if (action->isEnabled())
            action->trigger();
    }
}
