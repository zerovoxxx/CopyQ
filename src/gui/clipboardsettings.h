// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "common/appconfig.h"
#include "gui/clipboardbrowsershared.h"
#include <QPointer>
#include "gui/clipboardwindow.h"
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class ConfigurationManager;
class QDialog;

class ClipboardSettings final : public ClipboardWindow
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ClipboardSettingsWindow)
    QML_UNCREATABLE("Owned by MainWindow")
    Q_PROPERTY(QVariantList fields READ fields NOTIFY fieldsChanged)
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(QString language READ language NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)

public:
    explicit ClipboardSettings(const ClipboardBrowserSharedPtr &sharedData);
    ~ClipboardSettings() override;
    void open();
    QVariantList fields() const;
    QVariantList plugins() const;
    QVariantList languages() const;
    QString language() const;
    QVariantMap theme() const;
    QString error() const { return m_error; }
    Q_INVOKABLE bool setValue(const QString &name, const QVariant &value);
    Q_INVOKABLE void setLanguage(const QString &language);
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void apply(bool close = false);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setPluginEnabled(const QString &id, bool enabled);
    Q_INVOKABLE void movePlugin(const QString &id, int step);
    Q_INVOKABLE void openPage(const QString &page);
    Q_INVOKABLE void changeEncryptionPassword();

signals:
    void fieldsChanged();
    void pluginsChanged();
    void stateChanged();
    void configurationChanged(AppConfig *config);
    void finished();

protected:
    bool event(QEvent *event) override;

private:
    ClipboardBrowserSharedPtr m_sharedData;
    std::unique_ptr<ConfigurationManager> m_configuration;
    QPointer<QDialog> m_panel;
    QPointer<QWidget> m_panelBody;
    QString m_error;
};
