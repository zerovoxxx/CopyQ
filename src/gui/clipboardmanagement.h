// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "gui/clipboardpalettemodel.h"
#include "gui/menuitems.h"

#include <QFont>
#include <QQuickView>
#include <QSet>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QMenu;

class ClipboardManagement final : public QQuickView
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ClipboardManagementWindow)
    QML_UNCREATABLE("Owned by MainWindow")
    Q_PROPERTY(ClipboardPaletteModel *history READ history CONSTANT)
    Q_PROPERTY(QStringList tabs READ tabs NOTIFY sourceChanged)
    Q_PROPERTY(QVariantList tabTree READ tabTree NOTIFY sourceChanged)
    Q_PROPERTY(QString selectedTabPath READ selectedTabPath NOTIFY sourceChanged)
    Q_PROPERTY(bool selectedTabIsGroup READ selectedTabIsGroup NOTIFY sourceChanged)
    Q_PROPERTY(QVariant iconFont READ iconFont CONSTANT)
    Q_PROPERTY(QString tabName READ tabName NOTIFY sourceChanged)
    Q_PROPERTY(int currentTabIndex READ currentTabIndex NOTIFY sourceChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool monitoring READ monitoring NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QVariantMap tabProperties READ tabProperties NOTIFY sourceChanged)
    Q_PROPERTY(int maxItemCount READ maxItemCount NOTIFY sourceChanged)
    Q_PROPERTY(bool storeItems READ storeItems NOTIFY sourceChanged)
    Q_PROPERTY(int encryptedExpireSeconds READ encryptedExpireSeconds NOTIFY sourceChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QVariantList actions READ actions NOTIFY commandsChanged)
    Q_PROPERTY(QVariantMap options READ options NOTIFY stateChanged)

public:
    enum Action {
        File_New = Actions::File_New,
        File_Import = Actions::File_Import,
        File_Export = Actions::File_Export,
        File_Preferences = Actions::File_Preferences,
        File_Commands = Actions::File_Commands,
        File_ShowClipboardContent = Actions::File_ShowClipboardContent,
        File_ShowPreview = Actions::File_ShowPreview,
        File_ToggleClipboardStoring = Actions::File_ToggleClipboardStoring,
        File_ProcessManager = Actions::File_ProcessManager,
        File_Exit = Actions::File_Exit,
        Edit_SortSelectedItems = Actions::Edit_SortSelectedItems,
        Edit_ReverseSelectedItems = Actions::Edit_ReverseSelectedItems,
        Edit_PasteItems = Actions::Edit_PasteItems,
        Edit_CopySelectedItems = Actions::Edit_CopySelectedItems,
        Item_MoveToClipboard = Actions::Item_MoveToClipboard,
        Item_ShowContent = Actions::Item_ShowContent,
        Item_Remove = Actions::Item_Remove,
        Item_Edit = Actions::Item_Edit,
        Item_EditNotes = Actions::Item_EditNotes,
        Item_EditWithEditor = Actions::Item_EditWithEditor,
        Item_Action = Actions::Item_Action,
        Item_MoveUp = Actions::Item_MoveUp,
        Item_MoveDown = Actions::Item_MoveDown,
        Item_MoveToTop = Actions::Item_MoveToTop,
        Item_MoveToBottom = Actions::Item_MoveToBottom,
        Tabs_ChangeTabIcon = Actions::Tabs_ChangeTabIcon,
        Help_Help = Actions::Help_Help,
        Help_ShowLog = Actions::Help_ShowLog,
        Help_About = Actions::Help_About
    };
    Q_ENUM(Action)

    explicit ClipboardManagement(ItemFactory *factory);
    ~ClipboardManagement() override;
    ClipboardPaletteModel *history() { return &m_model; }
    const QStringList &tabs() const { return m_tabs; }
    QVariantList tabTree() const;
    QString selectedTabPath() const { return m_selectedTabPath; }
    bool selectedTabIsGroup() const;
    QVariant iconFont() const;
    QString tabName() const { return m_model.tabName(); }
    int currentTabIndex() const { return m_tabs.indexOf(tabName()); }
    const QString &error() const { return m_error; }
    bool monitoring() const { return m_monitoring; }
    QVariantMap theme() const { return m_theme; }
    QVariantMap tabProperties() const { return m_tabProperties; }
    int maxItemCount() const { return m_tabProperties.value(QStringLiteral("maxItemCount")).toInt(); }
    bool storeItems() const { return m_tabProperties.value(QStringLiteral("storeItems"), true).toBool(); }
    int encryptedExpireSeconds() const { return m_tabProperties.value(QStringLiteral("encryptedExpireSeconds")).toInt(); }
    QVariantList commands() const;
    QVariantList actions() const;
    QVariantMap options() const { return m_options; }
    void setOptions(const QVariantMap &options);
    Q_INVOKABLE void togglePreview();
    Q_INVOKABLE void clearHistory(int minutes);
    bool load();
    void open();
    void openAt(const QRect &geometry);
    void setSource(QAbstractItemModel *source, const QString &tabName,
                   const QStringList &tabs, const QVariantMap &properties);
    void setTabs(const QStringList &tabs);
    void setTabCounts(const QVariantMap &counts);
    void setTheme(const QVariantMap &theme);
    void setMonitoring(bool enabled);
    void setError(const QString &error);
    void setCommandMenu(QMenu *menu);
    void setActions(const MenuItems &items);
    Q_INVOKABLE void changeSource(const QString &tabName);
    Q_INVOKABLE void selectTabPath(const QString &path);
    Q_INVOKABLE void toggleGroup(const QString &path);
    Q_INVOKABLE QStringList collectionsInGroup(const QString &path) const;
    Q_INVOKABLE void renameGroup(const QString &path, const QString &name, const QStringList &members);
    Q_INVOKABLE void removeGroup(const QString &path, const QStringList &members);
    Q_INVOKABLE void moveGroup(const QString &path, int step);
    Q_INVOKABLE void sortGroup(const QString &path);
    Q_INVOKABLE QVariantList icons(const QString &filter) const;
    Q_INVOKABLE void saveIcon(const QString &icon, const QString &path);
    Q_INVOKABLE QString browseIcon();
    Q_INVOKABLE void triggerAction(int id);
    Q_INVOKABLE void triggerCommand(int index);
    Q_INVOKABLE void createTab(const QString &name);
    Q_INVOKABLE void renameTab(const QString &name, const QString &sourceTab = QString());
    Q_INVOKABLE void removeTab(const QString &sourceTab = QString());
    Q_INVOKABLE void moveTab(int step);
    Q_INVOKABLE void saveTabProperties(const QVariantMap &properties, const QString &sourceTab = QString());
    Q_INVOKABLE void transferItems(const QString &targetTab, bool move);
    Q_INVOKABLE void startDrag(int row);
    Q_INVOKABLE bool dropItems(const QString &targetTab, int row, bool move);

signals:
    void sourceRequested(const QString &tabName);
    void actionRequested(int id, const QString &tabName,
                         const QList<QPersistentModelIndex> &indexes, const QPersistentModelIndex &current);
    void tabRequested(const QString &operation, const QString &tabName, const QVariantMap &properties);
    void transferRequested(const QString &sourceTab, const QString &targetTab,
                           const QList<QPersistentModelIndex> &indexes, bool move, int row, bool *accepted);
    void dragRequested(const QString &tabName, const QList<QPersistentModelIndex> &indexes);
    void dataDropped(const QVariantMap &data, const QString &targetTab, int row, bool *accepted);
    void sourceChanged();
    void stateChanged();
    void themeChanged();
    void commandsChanged();
    void opened();
    void newTabRequested();
    void renameTabRequested();
    void removeTabRequested();
    void searchRequested();
    void itemMenuRequested();
    void iconRequested();
    void hideRequested();

protected:
    bool event(QEvent *event) override;

private:
    ClipboardPaletteModel m_model;
    QStringList m_tabs;
    QString m_selectedTabPath;
    QSet<QString> m_collapsedGroups;
    QString m_error;
    bool m_monitoring = true;
    QVariantMap m_theme;
    QVariantMap m_tabProperties;
    QVariantMap m_options;
    QVariantMap m_tabCounts;
    QPointer<QMenu> m_commands;
    MenuItems m_actions;
    QList<QPersistentModelIndex> m_dragIndexes;
    QString m_dragTab;
    QVariantMap m_dropData;
    bool m_explicitGeometry = false;
    bool m_restoreMaximized = false;
    QRect m_normalGeometry;
    void saveGeometry();
    QString geometryKey() const;
};
