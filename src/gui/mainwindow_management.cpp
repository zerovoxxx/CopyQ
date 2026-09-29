// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"

#include "common/appconfig.h"
#include "common/common.h"
#include "common/mimetypes.h"
#include "common/tabs.h"
#include "gui/clipboardbrowser.h"
#include "gui/clipboardbrowserplaceholder.h"
#include "gui/clipboarddialog.h"
#include "gui/clipboardmanagement.h"
#include "gui/clipboardpalette.h"
#include "gui/commandaction.h"
#include "gui/selectiondata.h"
#include "gui/tabwidget.h"
#include "item/itemfactory.h"
#include "platform/platformclipboard.h"
#include "ui_mainwindow.h"

#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMetaObject>
#include <algorithm>

namespace {
QModelIndexList validIndexes(const QList<QPersistentModelIndex> &indexes, const QAbstractItemModel *source)
{
    QModelIndexList result;
    for (const auto &index : indexes) {
        if (!index.isValid() || index.model() != source)
            return {};
        result.append(index);
    }
    std::sort(result.begin(), result.end());
    return result;
}
}

void MainWindow::showManagement()
{
    auto source = browser();
    if (!source || !source->isLoaded())
        return;
    updateFocusWindows();
    if (m_palette)
        m_palette->cancel();
    if (!m_management) {
        m_management = std::make_unique<ClipboardManagement>(m_sharedData->itemFactory);
        m_managementCommandMenu = new QMenu(this);
        connect(m_management.get(), &QWindow::visibleChanged, this, &MainWindow::updateQuickWindowState);
        connect(m_management.get(), &QWindow::activeChanged, this, &MainWindow::updateQuickWindowState);
        connect(m_management.get(), &ClipboardManagement::sourceRequested, this, [this](const QString &name) {
            const int i = findTabIndexExactMatch(name);
            if (i >= 0 && setCurrentTab(i)) {
                getPlaceholder(i)->createBrowserAgain();
                setManagementSource(name);
            }
        });
        connect(m_management.get(), &ClipboardManagement::actionRequested,
                this, &MainWindow::onManagementAction);
        connect(m_management.get(), &ClipboardManagement::tabRequested,
                this, &MainWindow::onManagementTab);
        connect(m_management.get(), &ClipboardManagement::transferRequested,
                this, &MainWindow::transferManagementItems);
        connect(m_management->history(), &ClipboardPaletteModel::itemDisplayRequested,
                this, &MainWindow::onItemWidgetCreated);
        connect(m_management->history(), &ClipboardPaletteModel::selectionChanged,
                this, &MainWindow::updateManagementCommands);
        connect(m_management.get(), &ClipboardManagement::dragRequested, this,
            [this](const QString &name, const QList<QPersistentModelIndex> &indexes) {
                const int i = findTabIndexExactMatch(name);
                const auto source = i >= 0 ? browser(i) : nullptr;
                const auto selected = source ? validIndexes(indexes, source->model()) : QModelIndexList();
                if (selected.isEmpty())
                    return;
                QDrag drag(m_management.get());
                drag.setMimeData(createMimeData(source->copyIndexes(selected)));
                drag.exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
            });
        connect(m_management.get(), &ClipboardManagement::dataDropped, this,
            [this](const QVariantMap &data, const QString &name, int row, bool *accepted) {
                const int i = findTabIndexExactMatch(name);
                auto destination = i >= 0 ? browser(i) : nullptr;
                if (destination && destination->isLoaded()) {
                    if (row >= 0 && destination->model() == m_management->history()->sourceModel()) {
                        const auto index = m_management->history()->sourceIndex(row);
                        row = index.isValid() ? index.row() : destination->length();
                    }
                    *accepted = destination->add(data, row);
                }
            });
    }
    m_management->setTheme(theme().quickTheme());
    m_management->setActions(m_sharedData->menuItems);
    m_management->setMonitoring(isMonitoringEnabled());
    setManagementSource(source->tabName());
    m_management->open();
    if (m_management->status() != QQuickView::Ready)
        showError(m_management->error());
    else
        hide();
}

void MainWindow::setManagementSource(const QString &tabName)
{
    if (!m_management)
        return;
    const int i = findTabIndexExactMatch(tabName);
    auto source = i >= 0 ? browser(i) : nullptr;
    const auto properties = Tabs().tabProperties(tabName);
    m_management->setSource(source && source->isLoaded() ? source->model() : nullptr, tabName, tabs(), {
        {QStringLiteral("maxItemCount"), properties.maxItemCount},
        {QStringLiteral("storeItems"), properties.storeItems},
        {QStringLiteral("encryptedExpireSeconds"), properties.encryptedExpireSeconds}
    });
    m_management->setError(source && source->isLoaded() ? QString() : tr("Collection is unavailable or locked. Select it again to reload."));
    updateQuickWindowState();
}

bool MainWindow::isBrowserVisibleInQuickWindow(const ClipboardBrowser *source) const
{
    return source && ((m_management && m_management->isVisible() && m_management->history()->sourceModel() == source->model())
        || (m_palette && m_palette->isVisible() && m_palette->history()->sourceModel() == source->model()));
}

void MainWindow::updateQuickWindowState()
{
    for (int i = 0; i < ui->tabWidget->count(); ++i)
        getPlaceholder(i)->refreshActiveState();
}

QVariantMap MainWindow::managementSelectionData() const
{
    if (!m_management || !m_management->isVisible())
        return {};
    const auto model = m_management->history();
    const auto source = qobject_cast<ClipboardBrowser*>(model->sourceModel() ? model->sourceModel()->parent() : nullptr);
    auto data = source
        ? selectionData(*source, model->selectedIndex(), validIndexes(model->selectedIndexes(), source->model()))
        : QVariantMap{{mimeCurrentTab, model->tabName()}};
    // An explicit empty selection prevents scripts from resolving the hidden QListView.
    addSelectionData(&data, model->selectedIndexes());
    return data;
}

void MainWindow::onManagementAction(int id, const QString &tabName,
        const QList<QPersistentModelIndex> &indexes, const QPersistentModelIndex &current)
{
    if (!m_management)
        return;
    const int tabIndex = findTabIndexExactMatch(tabName);
    auto source = tabIndex >= 0 ? browser(tabIndex) : nullptr;
    const auto selected = source ? validIndexes(indexes, source->model()) : QModelIndexList();
    if (!indexes.isEmpty() && selected.size() != indexes.size()) {
        m_management->setError(tr("Selection is no longer available."));
        return;
    }
    switch (id) {
    case Actions::File_Preferences: openPreferences(); return;
    case Actions::File_Commands: openCommands(); return;
    case Actions::File_Import: importData(); return;
    case Actions::File_Export: exportData(); return;
    case Actions::File_ShowClipboardContent: showClipboardContent(); return;
    case Actions::File_ProcessManager: showProcessManagerDialog(); return;
    case Actions::File_ToggleClipboardStoring: toggleClipboardStoring(); return;
    case Actions::File_Exit: exit(); return;
    case Actions::Help_Help: openHelp(); return;
    case Actions::Help_ShowLog: openLogDialog(); return;
    case Actions::Help_About: openAboutDialog(); return;
    case Actions::Tabs_ChangeTabIcon: setTabIcon(tabName); return;
    case Actions::Tabs_NewTab: m_management->setError(tr("Use New in Collections to create a collection.")); return;
    case Actions::Tabs_NextTab: nextTab(); return;
    case Actions::Tabs_PreviousTab: previousTab(); return;
    default: break;
    }
    if (!source || !source->isLoaded()) {
        m_management->setError(tr("Collection is unavailable."));
        return;
    }
    if (id == Actions::File_New) {
        openItemEditor({}, mimeText, source);
        return;
    }
    if (id == Actions::Edit_PasteItems) {
        const auto data = m_clipboard->mimeData(ClipboardMode::Clipboard);
        if (data)
            source->add(cloneData(data), selected.isEmpty() ? 0 : selected.first().row());
        return;
    }
    if (selected.isEmpty())
        return;
    switch (id) {
    case Actions::Edit_CopySelectedItems:
        setClipboard(source->copyIndexes(selected));
        break;
    case Actions::Item_MoveToClipboard: source->moveToClipboard(selected); break;
    case Actions::Item_Remove: {
        QString error;
        source->removeIndexes(selected, &error);
        m_management->setError(error);
        break;
    }
    case Actions::Edit_SortSelectedItems: source->sortItems(selected); break;
    case Actions::Edit_ReverseSelectedItems: source->reverseItems(selected); break;
    case Actions::Item_MoveToTop: source->move(selected, 0); break;
    case Actions::Item_MoveToBottom: source->move(selected, source->length()); break;
    case Actions::Item_MoveUp: source->move(selected, qMax(0, selected.first().row() - 1)); break;
    case Actions::Item_MoveDown: source->move(selected, qMin(source->length(), selected.last().row() + 2)); break;
    case Actions::Item_Edit:
    case Actions::Item_EditNotes:
        if (current.isValid() && current.model() == source->model())
            openItemEditor(current, id == Actions::Item_EditNotes ? QString(mimeItemNotes) : QString(mimeText), source);
        break;
    case Actions::Item_ShowContent:
        if (current.isValid() && current.model() == source->model()) {
            auto dialog = new ClipboardDialog(current, source->model(), this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
        break;
    case Actions::Item_EditWithEditor:
        for (const auto &index : selected) {
            if (!source->openEditor(index, mimeText)) {
                m_management->setError(tr("No external editor is configured for this item."));
                break;
            }
        }
        break;
    case Actions::Item_Action: openActionDialog(selectionData(*source, current, selected)); break;
    default: m_management->setError(tr("This action is not available in the manager yet.")); break;
    }
}

void MainWindow::onManagementTab(const QString &operation, const QString &name, const QVariantMap &values)
{
    if (!m_management)
        return;
    const int i = findTabIndexExactMatch(name);
    if (operation == QLatin1String("create")) {
        if (name.trimmed().isEmpty() || i >= 0) {
            m_management->setError(tr("Enter a unique collection name."));
            return;
        }
        addAndFocusTab(name);
        setManagementSource(name);
    } else if (operation == QLatin1String("rename")) {
        const auto newName = values.value(QStringLiteral("name")).toString();
        if (i < 0 || newName.trimmed().isEmpty() || (newName != name && tabs().contains(newName))) {
            m_management->setError(tr("Enter a unique collection name."));
            return;
        }
        renameTab(newName, i);
        setManagementSource(ui->tabWidget->tabName(i));
    } else if (operation == QLatin1String("remove")) {
        removeTab(false, i); // The QML dialog already confirmed the exact collection.
        setManagementSource(ui->tabWidget->tabName(ui->tabWidget->currentIndex()));
    } else if (operation == QLatin1String("move")) {
        auto names = tabs();
        const int target = i + values.value(QStringLiteral("step")).toInt();
        if (i >= 0 && target >= 0 && target < names.size()) {
            names.move(i, target);
            ui->tabWidget->setTabsOrder(names);
            setCurrentTab(target);
            AppConfig config;
            doSaveTabPositions(&config);
            setManagementSource(name);
        }
    } else if (operation == QLatin1String("properties") && i >= 0) {
        Tabs properties;
        auto tab = properties.tabProperties(name);
        tab.maxItemCount = qBound(0, values.value(QStringLiteral("maxItemCount")).toInt(), Config::maxItems);
        tab.storeItems = values.value(QStringLiteral("storeItems"), true).toBool();
        tab.encryptedExpireSeconds = qMax(0, values.value(QStringLiteral("encryptedExpireSeconds")).toInt());
        properties.setTabProperties(tab);
        AppConfig config;
        properties.save(&config.settings(), tabs());
        createTab(name, MatchExactTabName, properties);
        setManagementSource(name);
    }
}

void MainWindow::transferManagementItems(const QString &sourceTab, const QString &targetTab,
        const QList<QPersistentModelIndex> &indexes, bool moveItems, int row, bool *accepted)
{
    const int sourceTabIndex = findTabIndexExactMatch(sourceTab);
    const int targetTabIndex = findTabIndexExactMatch(targetTab);
    auto source = sourceTabIndex >= 0 ? browser(sourceTabIndex) : nullptr;
    auto target = targetTabIndex >= 0 ? browser(targetTabIndex) : nullptr;
    if (!source || !target || !source->isLoaded() || !target->isLoaded())
        return;
    auto selected = validIndexes(indexes, source->model());
    if (selected.isEmpty() || selected.size() != indexes.size())
        return;
    if (target->model() == m_management->history()->sourceModel() && row >= 0) {
        const auto index = m_management->history()->sourceIndex(row);
        row = index.isValid() ? index.row() : target->length();
    }
    QString error;
    *accepted = source->transferIndexes(target, selected, row, moveItems, &error);
    if (!*accepted && error.isEmpty())
        error = tr("Selection is no longer available.");
    m_management->setError(error);
}

void MainWindow::updateManagementCommands()
{
    if (!m_managementCommandMenu)
        return;
    interruptMenuCommandFilters(&m_managementMatchCommands);
    for (auto action : m_managementCommandMenu->actions())
        delete action;
    const auto model = m_management->history();
    const int i = findTabIndexExactMatch(m_management->tabName());
    auto source = i >= 0 ? getPlaceholder(i)->browser() : nullptr;
    const auto selected = source ? validIndexes(model->selectedIndexes(), source->model()) : QModelIndexList();
    if (!selected.isEmpty() && !model->filtering()) {
        auto data = selectionData(*source, model->selectedIndex(), selected);
        QList<QKeySequence> used;
        for (const auto &command : commandsForMenu(data, source->tabName(), m_menuCommands)) {
            auto action = new CommandAction(command, command.localizedName(), m_managementCommandMenu);
            QList<QKeySequence> shortcuts;
            for (const auto &text : command.shortcuts + command.globalShortcuts) {
                const QKeySequence shortcut(text);
                if (!used.contains(shortcut)) {
                    shortcuts.append(shortcut);
                    used.append(shortcut);
                }
            }
            action->setShortcuts(shortcuts);
            m_managementCommandMenu->addAction(action);
            addMenuMatchCommand(&m_managementMatchCommands, command.matchCmd, action);
            connect(action, &CommandAction::triggerCommand, this,
                [this](CommandAction *action, const QString &shortcut) { runManagementCommand(action->command(), shortcut); });
            connect(action, &QAction::changed, m_management.get(), [this]() {
                m_management->setCommandMenu(m_managementCommandMenu);
            });
        }
        runMenuCommandFilters(&m_managementMatchCommands, data);
    }
    m_management->setCommandMenu(m_managementCommandMenu);
}

void MainWindow::runManagementCommand(const Command &command, const QString &shortcut)
{
    const auto model = m_management->history();
    if (model->filtering())
        return;
    const int i = findTabIndexExactMatch(m_management->tabName());
    auto source = i >= 0 ? browser(i) : nullptr;
    const auto selected = source ? validIndexes(model->selectedIndexes(), source->model()) : QModelIndexList();
    if (selected.isEmpty())
        return;
    const auto addShortcut = [&shortcut](QVariantMap data) {
        if (!shortcut.isEmpty())
            data.insert(mimeShortcut, shortcut);
        return data;
    };
    if (!command.cmd.isEmpty()) {
        if (command.transform) {
            for (const auto &index : selected)
                action(addShortcut(selectionData(*source, index, {index})), command, index);
        } else {
            action(addShortcut(selectionData(*source, model->selectedIndex(), selected)), command, {});
        }
    }
    if (!command.tab.isEmpty() && command.tab != source->tabName()) {
        auto target = tab(command.tab);
        if (target) {
            for (auto it = selected.crbegin(); it != selected.crend(); ++it)
                target->addUnique(source->copyIndex(*it), ClipboardMode::Clipboard);
        }
    }
    if (command.remove && (command.tab.isEmpty() || command.tab != source->tabName())) {
        QString error;
        source->removeIndexes(selected, &error);
        m_management->setError(error);
    }
    if (command.hideWindow)
        m_management->hide();
}
