// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "gui/clipboardpalettemodel.h"
#include "platform/platformnativeinterface.h"

#include <QQuickView>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QMenu;

class ClipboardPalette final : public QQuickView
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ClipboardPaletteWindow)
    QML_UNCREATABLE("Owned by MainWindow")
    Q_PROPERTY(ClipboardPaletteModel *history READ history CONSTANT)
    Q_PROPERTY(QStringList tabs READ tabs NOTIFY sourceChanged)
    Q_PROPERTY(QString tabName READ tabName NOTIFY sourceChanged)
    Q_PROPERTY(int currentTabIndex READ currentTabIndex NOTIFY sourceChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QString enterLabel READ enterLabel NOTIFY commandsChanged)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)

public:
    explicit ClipboardPalette(ItemFactory *factory);
    ~ClipboardPalette() override;
    ClipboardPaletteModel *history() { return &m_model; }
    const QStringList &tabs() const { return m_tabs; }
    QString tabName() const { return m_model.tabName(); }
    int currentTabIndex() const { return m_tabs.indexOf(tabName()); }
    const QString &error() const { return m_error; }
    bool busy() const { return m_busy; }
    bool load();
    void open(QAbstractItemModel *source, const QString &tabName,
              const QStringList &tabs, const PlatformWindowPtr &target);
    void setHistorySource(QAbstractItemModel *source, const QString &tabName);
    PlatformWindowPtr target() const { return m_target; }
    QPersistentModelIndex pendingIndex() const { return m_pendingIndex; }
    QVariantMap pendingData() const { return m_pendingData; }
    quint64 activationId() const { return m_activationId; }
    bool beginCommand();
    bool pendingValid() const;
    void complete(const QString &error = QString());
    void setCommandMenu(QMenu *menu);
    QVariantList commands() const;
    QString enterLabel() const;
    QVariantMap theme() const { return m_theme; }
    void setTheme(const QVariantMap &theme) { m_theme = theme; emit themeChanged(); }
    Q_INVOKABLE void activate(bool paste = true, bool plainText = false, bool bypassDefault = false);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void changeSource(const QString &tabName);
    Q_INVOKABLE void openUrl(const QString &url);
    Q_INVOKABLE void previewFile();
    Q_INVOKABLE void editItem();
    Q_INVOKABLE void showPluginSettings();
    Q_INVOKABLE void showManagement() { cancel(); emit managementRequested(); }
    Q_INVOKABLE void triggerCommand(int index);

signals:
    void activationRequested(const QPersistentModelIndex &index, const QVariantMap &data, bool paste);
    void activationCancelled();
    void sourceRequested(const QString &tabName);
    void editorRequested(const QPersistentModelIndex &index);
    void pluginSettingsRequested();
    void managementRequested();
    void sourceChanged();
    void stateChanged();
    void errorOccurred(const QString &error);
    void commandsChanged();
    void opened();
    void themeChanged();

protected:
    bool event(QEvent *event) override;

private:
    void setError(const QString &error);
    bool composing() const;

    ClipboardPaletteModel m_model;
    PlatformWindowPtr m_target;
    QPersistentModelIndex m_pendingIndex;
    QVariantMap m_pendingData;
    QPointer<QMenu> m_commands;
    QStringList m_tabs;
    QString m_error;
    QVariantMap m_theme;
    bool m_busy = false;
    bool m_wasActive = false;
    quint64 m_activationId = 0;
};
