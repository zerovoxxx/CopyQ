// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "common/command.h"
#include "common/commandstore.h"
#include "gui/clipboardbrowsershared.h"

#include "gui/clipboardwindow.h"
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class ClipboardCommands final : public ClipboardWindow
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ClipboardCommandsWindow)
    QML_UNCREATABLE("Owned by MainWindow")
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QVariantList templates READ templates CONSTANT)
    Q_PROPERTY(QVariantList fields READ fields NOTIFY fieldsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY fieldsChanged)
    Q_PROPERTY(QString error READ error NOTIFY fieldsChanged)
    Q_PROPERTY(QVariantMap theme READ theme CONSTANT)
    Q_PROPERTY(bool modified READ modified NOTIFY commandsChanged)

public:
    explicit ClipboardCommands(const ClipboardBrowserSharedPtr &sharedData);
    ~ClipboardCommands() override;
    void open();
    QVariantList commands() const;
    QVariantList templates() const;
    QVariantList fields() const;
    int currentIndex() const { return m_current; }
    QString error() const { return m_error; }
    QVariantMap theme() const;
    bool modified() const { return m_commands != m_saved; }
    bool maybeClose(QWidget *parent);
    void addCommands(const Commands &commands);
    Q_INVOKABLE void select(int row);
    Q_INVOKABLE bool setField(const QString &name, const QVariant &value);
    Q_INVOKABLE void create();
    Q_INVOKABLE void addTemplate(int row);
    Q_INVOKABLE void remove(const QList<int> &rows);
    Q_INVOKABLE void move(int row, int step);
    Q_INVOKABLE QString exportSelected(const QList<int> &rows) const;
    Q_INVOKABLE bool importText(const QString &text);
    Q_INVOKABLE void importFile();
    Q_INVOKABLE void exportFile(const QList<int> &rows);
    Q_INVOKABLE void copy(const QList<int> &rows);
    Q_INVOKABLE void paste();
    Q_INVOKABLE void editCode(const QString &field);
    Q_INVOKABLE bool apply(bool close = false);
    Q_INVOKABLE void cancel();

signals:
    void commandsChanged();
    void fieldsChanged();
    void commandsSaved();
    void clipboardRequested(const QVariantMap &data);
    void finished();

protected:
    bool event(QEvent *event) override;

private:
    Commands selectedCommands(const QList<int> &rows) const;
    ClipboardBrowserSharedPtr m_sharedData;
    Commands m_commands;
    Commands m_saved;
    Commands m_templates;
    int m_current = -1;
    QString m_error;
};
