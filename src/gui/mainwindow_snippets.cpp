// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"
#include "gui/clipboardsnippets.h"
#include "gui/clipboardmanagement.h"
#include "gui/clipboardbrowser.h"
#include "gui/clipboardpalette.h"
#include "common/action.h"
#include "gui/actionhandler.h"
#include "common/appconfig.h"
#include "common/commandstore.h"
#include "common/config.h"
#include "common/copyqimport.h"
#include "gui/passwordprompt.h"
#include "gui/notification.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QRegularExpression>
#include "common/contenttype.h"
#include "common/common.h"
#include "common/historypolicy.h"
#include "common/mimetypes.h"
#include "item/snippetstore.h"
#include "item/itemfactory.h"
#include "platform/platforminput.h"
#include "platform/platformclipboard.h"
#include "platform/platformwindow.h"
#include "scriptable/scriptoverrides.h"
#include <QApplication>
#include <QMimeData>
#include <QTextBoundaryFinder>
#include <QUuid>

namespace {
const QString sourceMime = QStringLiteral(COPYQ_MIME_PRIVATE_PREFIX "snippet-source");
}

SnippetStore *MainWindow::snippetStore()
{
    if (!m_snippetStore) {
        m_snippetStore = std::make_unique<SnippetStore>(configurationFilePath("_snippets.dat"));
        if (!m_snippetStore->load(m_sharedData->encryptionKey)) showError(m_snippetStore->error());
        connect(m_snippetStore.get(), &SnippetStore::changed, this, &MainWindow::loadSnippetSettings);
    }
    return m_snippetStore.get();
}

bool MainWindow::showSnippets()
{
    if (m_sharedData->tabsEncrypted && !m_sharedData->encryptionKey.isValid()) {
        const auto key = m_sharedData->passwordPrompt->prompt(PasswordSource::UseEnvAndKeychain);
        if (!key.isValid()) return false;
        m_sharedData->encryptionKey = key;
    }
    auto store = snippetStore();
    auto target = platformNativeInterface()->getCurrentWindow();
    if (!allowedSnippetTarget(target)) target = m_windowForMainPaste;
    if (!m_snippets) {
        m_snippets = std::make_unique<ClipboardSnippets>(store, m_sharedData);
        m_snippets->setContext([this] { return snippetContext(); });
        connect(m_snippets.get(), &ClipboardSnippets::outputRequested, this,
            [this](const QVariantMap &data, int cursor, bool paste, const PlatformWindowPtr &window) {
                writeSnippetOutput(data, cursor, paste ? 2 : 1, window, 0);
            });
        connect(m_snippets.get(), &ClipboardSnippets::copyQImportRequested, this, &MainWindow::importCopyQProfile);
        connect(m_snippets.get(), &ClipboardSnippets::commandRequested, this,
            [this](const QString &name, const QVariantMap &data, const PlatformWindowPtr &window) { runSnippetCommand(name, data, window, 0); });
    }
    loadSnippetSettings();
    if (m_palette) m_palette->cancel();
    m_snippets->open(target);
    return m_snippets->isVisible();
}

void MainWindow::saveHistorySnippet(const QPersistentModelIndex &index)
{
    if (!index.isValid()) return;
    auto store = snippetStore();
    const auto before = store->document();
    QString collection;
    if (!store->collections().isEmpty()) collection = store->collections().first().toMap().value(QStringLiteral("id")).toString();
    else collection = store->createCollection(tr("Saved clips"));
    const auto id = store->createSnippet(collection, m_sharedData->itemFactory->data(index));
    if (id.isEmpty() || (m_sharedData->tabsEncrypted && !m_sharedData->encryptionKey.isValid()) || !store->save(m_sharedData->encryptionKey)) {
        const auto error = store->error(); store->setDocument(before); showError(error); return;
    }
    showSnippets(); m_snippets->setCollectionId(collection); m_snippets->select(id);
}

SnippetContext MainWindow::snippetContext()
{
    SnippetContext context;
    context.now = QDateTime::currentDateTime();
    context.locale = QLocale();
    const auto clipboard = getClipboardData(ClipboardMode::Clipboard);
    if (clipboard) context.clipboard = clipboard->text();
    const AppConfig config;
    auto source = tab(config.option<Config::clipboard_tab>());
    if (source && source->isLoaded()) {
        for (int row = 0; row < source->model()->rowCount(); ++row)
            context.history.append(QString::fromUtf8(source->model()->index(row, 0).data(contentType::data).toMap().value(mimeText).toByteArray()));
    }
    context.reference = [this](const QString &keyword) -> QVariantMap {
        auto store = snippetStore();
        QVariantMap match;
        for (const auto &value : store->snippets()) {
            const auto snippet = value.toMap();
            if (store->effectiveKeyword(snippet) == keyword) {
                if (!match.isEmpty()) return {};
                match = snippet.value(QStringLiteral("data")).toMap();
            }
        }
        return match;
    };
    return context;
}

bool MainWindow::allowedSnippetTarget(const PlatformWindowPtr &target) const
{
    if (!target || !target->isValid()) return false;
    for (const auto window : QGuiApplication::allWindows()) if (target->matchesWindow(window)) return false;
    for (const auto widget : QApplication::topLevelWidgets()) if (target->matchesWidget(widget)) return false;
    const auto application = target->getApplicationId();
    if (application.isEmpty()) return false;
    const auto excluded = m_snippetStore ? m_snippetStore->settings().value(QStringLiteral("excludedApps")).toStringList() : QStringList();
    return !excluded.contains(application, Qt::CaseInsensitive)
        && !AppConfig().option<Config::clipboard_history_ignore_apps>().contains(application, Qt::CaseInsensitive);
}

void MainWindow::loadSnippetSettings()
{
    auto store = snippetStore();
    if (!store->writable() && m_sharedData->encryptionKey.isValid()) store->load(m_sharedData->encryptionKey);
    if (!m_snippetClock.isValid()) m_snippetClock.start();
    const auto settings = store->settings();
    const bool enabled = store->writable() && (!m_sharedData->tabsEncrypted || m_sharedData->encryptionKey.isValid())
        && (settings.value(QStringLiteral("autoExpand")).toBool() || settings.value(QStringLiteral("merge")).toBool());
    if (!m_platformInput) {
        m_platformInput = createPlatformInput();
        connect(m_platformInput.get(), &PlatformInput::input, this, [this](const QString &text, int key, Qt::KeyboardModifiers modifiers) {
            // Take the prior content at modifier down, before the target handles C.
            if (key == Qt::Key_Control && modifiers == Qt::ControlModifier && snippetStore()->settings().value(QStringLiteral("merge")).toBool()) {
                const auto clipboard = getClipboardData(ClipboardMode::Clipboard);
                m_snippetCopyClipboard = clipboard ? cloneData(clipboard) : QVariantMap();
            }
            QTimer::singleShot(0, this, [this, text, key, modifiers] { onSnippetInput(text, key, modifiers); });
        });
        connect(m_platformInput.get(), &PlatformInput::reset, this, &MainWindow::resetSnippetInput, Qt::QueuedConnection);
        m_snippetClipboardTimer.setSingleShot(true);
        connect(&m_snippetClipboardTimer, &QTimer::timeout, this, [this] { resetSnippetInput(); showError(tr("Snippet clipboard write timed out; no keyword was deleted.")); });
    }
    if (enabled) {
        const bool started = m_platformInput->start();
        if (m_snippets) m_snippets->setInputStatus(started ? tr("Monitoring enabled. Expansion pauses during unconfirmed IME or secure input.") : m_platformInput->error());
    } else {
        m_platformInput->stop(); resetSnippetInput();
        if (m_snippets) m_snippets->setInputStatus(tr("Automatic expansion and copy merging are disabled."));
    }
}

void MainWindow::resetSnippetInput()
{
    m_snippetInput = QStringLiteral("_"); ++m_snippetGeneration; m_snippetInputTarget.reset();
    m_clipboardMerge.reset(); m_snippetCopyClipboard.clear(); m_snippetClipboardTimer.stop(); m_snippetProviderId = -1;
}

void MainWindow::onSnippetInput(const QString &text, int key, Qt::KeyboardModifiers modifiers)
{
    if (key == Qt::Key_Control && modifiers == Qt::NoModifier) {
        m_clipboardMerge.endGesture(); m_snippetCopyClipboard.clear(); m_snippetInput = QStringLiteral("_"); return;
    }
    const auto target = platformNativeInterface()->getCurrentWindow();
    if (!m_platformInput || !m_platformInput->isSafe() || !allowedSnippetTarget(target) || !target->isActive()) { resetSnippetInput(); return; }
    if (m_snippetInputTarget && !m_snippetInputTarget->isActive()) resetSnippetInput();
    m_snippetInputTarget = target;
    ++m_snippetGeneration;
    if (m_snippetProviderId >= 0) { resetSnippetInput(); return; }
    const auto settings = snippetStore()->settings();
    if (key == Qt::Key_C && modifiers == Qt::ControlModifier && settings.value(QStringLiteral("merge")).toBool()) {
        m_snippetInput = QStringLiteral("_");
        const auto current = getClipboardData(ClipboardMode::Clipboard);
        const auto data = current ? cloneData(current) : QVariantMap();
        m_clipboardMerge.copy(m_snippetClock.elapsed(), target->getApplicationId() + QLatin1Char('\n') + target->getTitle(), m_snippetCopyClipboard.isEmpty() ? data : m_snippetCopyClipboard);
        if (m_clipboardMerge.armed()) {
            QTimer::singleShot(100, this, [this, target] {
                if (!target->isActive() || !allowedSnippetTarget(target)) { m_clipboardMerge.reset(); return; }
                const auto current = getClipboardData(ClipboardMode::Clipboard);
                const auto data = current ? cloneData(current) : QVariantMap();
                // Clipboard history can be disabled while copy merging is enabled.
                m_clipboardMerge.observe(m_snippetClock.elapsed(), target->getApplicationId() + QLatin1Char('\n') + target->getTitle(), data,
                    data.contains(sourceMime) || data.contains(mimeOwner),
                    HistoryPolicy::accepts(data, {QStringLiteral("text")}, AppConfig().option<Config::clipboard_history_ignore_apps>()));
                const auto merged = m_clipboardMerge.take(m_snippetClock.elapsed(), target->getApplicationId() + QLatin1Char('\n') + target->getTitle(), data,
                    snippetStore()->settings().value(QStringLiteral("separator"), QStringLiteral("\n")).toString());
                if (!merged.isEmpty()) writeSnippetOutput(merged, -1, 4, target, 0);
            });
        }
        return;
    }
    m_clipboardMerge.reset();
    if (modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) { m_snippetInput = QStringLiteral("_"); return; }
    if (key == Qt::Key_Backspace) {
        QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme, m_snippetInput); boundary.toEnd();
        const auto previous = boundary.toPreviousBoundary();
        if (previous >= 0) m_snippetInput.truncate(previous);
        return;
    }
    if (text.isEmpty() || text.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f]")))) { m_snippetInput = QStringLiteral("_"); return; }
    m_snippetInput += text;
    // Keywords longer than this are rejected by the editor/data validator.
    if (m_snippetInput.size() > 1024) m_snippetInput = QStringLiteral("_") + m_snippetInput.right(900);
    if (!settings.value(QStringLiteral("autoExpand")).toBool()) return;
    const auto id = matchSnippet(m_snippetInput, snippetStore()->snippets(), snippetStore()->collections(), settings.value(QStringLiteral("anywhere")).toBool());
    if (id.isEmpty()) return;
    const auto snippet = snippetStore()->snippet(id);
    const auto result = renderSnippet(snippet.value(QStringLiteral("data")).toMap(), snippetContext());
    if (!result.valid()) { resetSnippetInput(); showError(result.error); return; }
    const int erase = snippetGraphemeCount(snippetStore()->effectiveKeyword(snippet));
    const auto command = snippet.value(QStringLiteral("command")).toString();
    if (!command.isEmpty()) runSnippetCommand(command, result.data, target, erase);
    else writeSnippetOutput(result.data, result.cursor, 3, target, erase);
}

void MainWindow::runSnippetCommand(const QString &name, const QVariantMap &data, const PlatformWindowPtr &target, int erase)
{
    Command selected;
    int count = 0;
    for (const auto &command : loadAllCommands()) {
        if (command.enable && command.name == name && !command.isScript && !command.cmd.isEmpty()) { selected = command; ++count; }
    }
    if (count != 1 || (erase && selected.wait)) { showError(tr("Choose one enabled, unambiguous QClip command. Automatic triggers cannot open an action dialog.")); return; }
    if (!allowedSnippetTarget(target)) { showError(tr("The Snippet command target is unavailable.")); return; }
    const auto generation = m_snippetGeneration;
    if (!erase) { if (m_snippets) m_snippets->hide(); target->raise(); }
    QTimer::singleShot(erase ? 30 : qMax(100, AppConfig().option<Config::window_wait_after_raised_ms>()), this,
        [this, selected, data, target, generation, erase] {
            const auto guard = [this, target, generation] { return m_snippetGeneration == generation && target->isActive() && allowedSnippetTarget(target); };
            if (!guard() || (erase && !m_platformInput->sendKey(Qt::Key_Backspace, erase, guard))) { showError(tr("Snippet command cancelled because its target or input changed.")); return; }
            auto input = data;
            input.insert(mimeWindowTitle, target->getTitle());
            const auto act = action(input, selected, {});
            if (!act) return;
            m_snippetActions.insert(act->id(), {target, generation});
            connect(act, &Action::actionFinished, this, [this](Action *finished) { m_snippetActions.remove(finished->id()); });
            m_snippetInput = QStringLiteral("_");
        });
}

void MainWindow::writeSnippetOutput(const QVariantMap &data, int cursor, int kind, const PlatformWindowPtr &target, int erase)
{
    if (data.isEmpty() || m_snippetProviderId >= 0) return;
    if (kind != 1 && (!allowedSnippetTarget(target) || (kind != 2 && !target->isActive()))) { showError(tr("The Snippet target changed; output was cancelled.")); return; }
    if (kind == 2 && cursor >= 0 && !m_platformInput->start()) { showError(m_platformInput->error()); return; }
    m_snippetTarget = target; m_snippetCursor = cursor; m_snippetKind = kind; m_snippetErase = erase;
    const auto previous = getClipboardData(ClipboardMode::Clipboard);
    m_snippetClipboardBackup = previous ? cloneData(previous) : QVariantMap();
    m_snippetData = data;
    m_snippetData.insert(sourceMime, QUuid::createUuid().toByteArray());
    m_snippetPendingGeneration = m_snippetGeneration;
    setClipboard(m_snippetData, ClipboardMode::Clipboard);
    m_snippetProviderId = m_provideClipboardActionId;
    m_snippetClipboardTimer.start(5000);
    const auto provider = m_sharedData->actions->findAction(m_snippetProviderId);
    if (provider) connect(provider, &Action::actionFinished, this, [this](Action *action) {
        if (action->id() != m_snippetProviderId) return;
        // A public paste() override can intentionally replace this provider via copy().
        for (const auto &pending : m_snippetActions)
            if (pending.generation == m_snippetPendingGeneration) return;
        resetSnippetInput(); showError(tr("Snippet clipboard provider failed; no replacement was sent."));
    });
}

void MainWindow::finishSnippetClipboard(int providerId)
{
    if (providerId != m_snippetProviderId) return;
    m_snippetClipboardTimer.stop();
    const int kind = m_snippetKind;
    if (kind == 1 || kind == 4) {
        m_snippetProviderId = -1;
        if (kind == 4) {
            if (snippetStore()->settings().value(QStringLiteral("sound")).toBool()) QApplication::beep();
            auto notification = createNotification(QStringLiteral("snippet-merge"));
            notification->setTitle(tr("QClip")); notification->setMessage(tr("Copied text merged."));
            notification->setInterval(1500); notification->show();
            emit snippetMerged();
        }
        return;
    }
    const auto target = m_snippetTarget;
    if (kind == 2) { if (m_snippets) m_snippets->hide(); if (target) target->raise(); }
    const auto generation = m_snippetPendingGeneration;
    const auto guard = [this, target, providerId, generation] {
        const auto clipboard = getClipboardData(ClipboardMode::Clipboard);
        return m_snippetProviderId == providerId && m_registeredClipboardProviderId == providerId
            && m_provideClipboardActionId == providerId && m_snippetGeneration == generation
            && allowedSnippetTarget(target) && target->isActive()
            && clipboard && clipboard->data(sourceMime) == m_snippetData.value(sourceMime).toByteArray()
            && (m_snippetKind != 3 || m_platformInput->isSafe());
    };
    QTimer::singleShot(kind == 2 ? qMax(100, AppConfig().option<Config::window_wait_after_raised_ms>()) : 30, this,
        [this, target, providerId, guard, kind] {
            if (!guard()) { if (m_snippetProviderId != providerId) return; resetSnippetInput(); showError(tr("Snippet target or input changed; no replacement was sent.")); return; }
            bool success = !m_snippetErase || m_platformInput->sendKey(Qt::Key_Backspace, m_snippetErase, guard);
            // Allow the target application to process deletion before it receives paste.
            QTimer::singleShot(30, this, [this, target, providerId, guard, kind, success] {
                if (m_snippetProviderId != providerId) return;
                const auto body = QString::fromUtf8(m_snippetData.value(mimeText).toByteArray());
                const int left = m_snippetCursor < 0 ? 0 : snippetGraphemeCount(body.mid(m_snippetCursor));
                const auto complete = [this, providerId, guard, kind, left](bool pasted) {
                  QTimer::singleShot(80, this, [this, providerId, guard, kind, pasted, left] {
                    if (m_snippetProviderId != providerId) return;
                    const bool success = pasted && (left == 0 || (guard() && m_platformInput->sendKey(Qt::Key_Left, left, guard)));
                    m_snippetProviderId = -1; m_snippetInput = QStringLiteral("_");
                    if (kind == 2) loadSnippetSettings();
                    if (!success) showError(tr("Snippet insertion failed; input was not replayed."));
                    if (kind == 3 && snippetStore()->settings().value(QStringLiteral("restoreClipboard"), true).toBool()) {
                        const auto snapshot = m_snippetClipboardBackup;
                        const auto token = m_snippetData.value(sourceMime).toByteArray();
                        const auto generation = m_clipboardGeneration;
                        QTimer::singleShot(500, this, [this, providerId, snapshot, token, generation] {
                            const auto clipboard = getClipboardData(ClipboardMode::Clipboard);
                            if (m_clipboardGeneration == generation && m_provideClipboardActionId == providerId
                                    && clipboard && clipboard->data(sourceMime) == token) setClipboard(snapshot, ClipboardMode::Clipboard);
                        });
                    }
                  });
                };
                if (!success || !guard()) { complete(false); return; }
                if (isScriptOverridden(ScriptOverrides::Paste)) {
                    const auto act = runScript(QStringLiteral("paste()"), m_snippetData);
                    m_snippetActions.insert(act->id(), {target, m_snippetGeneration});
                    connect(act, &Action::actionFinished, this, [this, complete](Action *finished) {
                        m_snippetActions.remove(finished->id());
                        complete(!finished->actionFailed() && finished->exitCode() == 0);
                    });
                } else complete(target->pasteFromClipboardSafely(guard));
            });
        });
}

void MainWindow::importCopyQProfile(const QString &directory)
{
    Encryption::EncryptionKey key;
    const auto configPath = copyQConfigurationPath(directory);
    if (configPath.isEmpty()) { showError(tr("Select a directory containing exactly one CopyQ profile.")); return; }
    QSettings config(configPath, QSettings::IniFormat);
    if (config.value(QStringLiteral("Options/encrypt_tabs")).toBool()) {
        if (!m_sharedData->tabsEncrypted || !m_sharedData->encryptionKey.isValid()) { showError(tr("Enable and unlock QClip encryption before importing an encrypted CopyQ profile.")); return; }
        if (!Encryption::initialize()) { showError(tr("Encryption support is unavailable.")); return; }
        const auto password = promptForNewPassword(tr("CopyQ import"), tr("Enter the source CopyQ password:"), PasswordPromptType::AskOnce, this);
        if (password.isEmpty()) return;
        const auto base = configPath.left(configPath.lastIndexOf(QLatin1Char('.')));
        QFile wrapped(base + QStringLiteral("wrapped_dek.dat"));
        QFile salt(base + QStringLiteral("kek_salt.dat"));
        if (wrapped.exists() || salt.exists()) {
            if (!wrapped.open(QIODevice::ReadOnly) || !salt.open(QIODevice::ReadOnly)) { showError(tr("Missing CopyQ encryption material.")); return; }
            key = Encryption::EncryptionKey(password, Encryption::SecureArray(wrapped.readAll()), Encryption::Salt(salt.readAll()));
        } else key = Encryption::EncryptionKey(password);
        if (!key.isValid()) { showError(tr("Unable to unlock CopyQ profile.")); return; }
    }
    QByteArray archive; QString error;
    if (!prepareCopyQImport(directory, &archive, &error, key)) { showError(error); return; }
    QTemporaryDir isolated;
    const auto path = isolated.filePath(QStringLiteral("copyq.cpq"));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(archive) != archive.size() || !file.commit()) { showError(tr("Unable to stage CopyQ import.")); return; }
    if (!importDataFrom(path, ImportOptions::All)) showError(tr("CopyQ import failed. The source profile was preserved."));
}
