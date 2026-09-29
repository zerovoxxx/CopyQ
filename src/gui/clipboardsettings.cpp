// SPDX-License-Identifier: GPL-3.0-or-later
#include "clipboardsettings.h"
#include "configurationmanager.h"
#include "common/appconfig.h"
#include "gui/windowgeometryguard.h"

#include <QDialog>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QQuickItem>
#include <QVBoxLayout>

ClipboardSettings::ClipboardSettings(const ClipboardBrowserSharedPtr &sharedData)
    : m_sharedData(sharedData)
    , m_configuration(std::make_unique<ConfigurationManager>(sharedData))
{
    setObjectName(QStringLiteral("clipboard_settings"));
    setTitle(tr("QClip — Settings"));
    setMinimumSize(QSize(760, 520));
    setResizeMode(QQuickView::SizeRootObjectToView);
    m_configuration->initializeLanguages();
    connect(m_configuration.get(), &ConfigurationManager::configurationChanged,
            this, &ClipboardSettings::configurationChanged);
    connect(m_configuration.get(), &ConfigurationManager::error, this, [this](const QString &error) {
        m_error = error;
        emit stateChanged();
    });
}

ClipboardSettings::~ClipboardSettings()
{
    QQuickView::setSource(QUrl());
}

void ClipboardSettings::open()
{
    if (status() != QQuickView::Ready) {
        setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(this)}});
        QQuickView::setSource(QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/Settings.qml")));
    }
    if (status() != QQuickView::Ready) {
        m_error = tr("Unable to load settings.");
        emit stateChanged();
        return;
    }
    if (!isVisible() && screen()) {
        const auto area = screen()->availableGeometry();
        const QSize size(qMin(980, area.width()), qMin(720, area.height()));
        setGeometry(QRect(area.center() - QPoint(size.width()/2, size.height()/2), size));
    }
    show();
    raise();
    requestActivate();
    rootObject()->forceActiveFocus();
}

QVariantList ClipboardSettings::fields() const { return m_configuration->optionFields(); }
QVariantList ClipboardSettings::plugins() const { return m_configuration->pluginFields(); }
QVariantList ClipboardSettings::languages() const { return m_configuration->languages(); }
QString ClipboardSettings::language() const { return m_configuration->language(); }
QVariantMap ClipboardSettings::theme() const { return m_sharedData->theme.quickTheme(); }

bool ClipboardSettings::setValue(const QString &name, const QVariant &value)
{
    const bool valid = m_configuration->setDraftValue(name, value);
    m_error = valid ? QString() : tr("Invalid value for %1.").arg(name);
    emit stateChanged();
    return valid;
}

void ClipboardSettings::setLanguage(const QString &language)
{
    m_configuration->setLanguage(language);
    emit stateChanged();
}

void ClipboardSettings::resetDefaults()
{
    m_configuration->resetDraft();
    emit fieldsChanged();
    emit stateChanged();
}

void ClipboardSettings::apply(bool close)
{
    if (!m_error.isEmpty())
        return;
    m_configuration->applyDraft();
    emit fieldsChanged();
    emit pluginsChanged();
    emit stateChanged();
    if (close) {
        if (m_panel)
            m_panel->hide();
        hide();
        emit finished();
    }
}

void ClipboardSettings::cancel()
{
    if (m_panel)
        m_panel->hide();
    hide();
    // Recreate on next open; plugin widgets must not survive their draft.
    emit finished();
}

bool ClipboardSettings::event(QEvent *event)
{
    if (event->type() == QEvent::Close)
        cancel();
    return QQuickView::event(event);
}

void ClipboardSettings::setPluginEnabled(const QString &id, bool enabled)
{
    m_configuration->setPluginEnabled(id, enabled);
    emit pluginsChanged();
}

void ClipboardSettings::movePlugin(const QString &id, int step)
{
    m_configuration->movePlugin(id, step);
    emit pluginsChanged();
}

void ClipboardSettings::openPage(const QString &page)
{
    auto body = m_configuration->settingsPage(page);
    if (!body)
        return;
    if (m_panelBody) {
        m_panelBody->hide();
        m_panelBody->setParent(m_configuration.get());
    }
    if (m_panel)
        delete m_panel;
    m_panel = new QDialog(m_configuration.get());
    m_panel->setObjectName(QStringLiteral("qclip_settings_native"));
    m_panel->setWindowTitle(tr("QClip — %1").arg(page));
    m_panel->resize(920, 680);
    auto layout = new QVBoxLayout(m_panel);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);
    auto heading = new QLabel(tr("%1 settings").arg(page), m_panel);
    auto font = m_sharedData->theme.font(QStringLiteral("font"));
    font.setBold(true);
    heading->setFont(font);
    layout->addWidget(heading);
    auto hint = new QLabel(tr("Changes are applied with Settings. Cancel Settings to discard this draft."), m_panel);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    m_panelBody = body;
    layout->addWidget(body, 1);
    body->show();
    auto buttons = new QHBoxLayout;
    buttons->addStretch();
    auto back = new QPushButton(tr("Back to Settings"), m_panel);
    auto apply = new QPushButton(tr("Apply"), m_panel);
    buttons->addWidget(back);
    buttons->addWidget(apply);
    layout->addLayout(buttons);
    connect(back, &QPushButton::clicked, this, [this]() { m_panel->hide(); open(); });
    connect(apply, &QPushButton::clicked, this, [this]() { this->apply(); });
    m_sharedData->theme.decorateMainWindow(m_panel);
    m_panel->show();
    raiseWindow(m_panel);
}

void ClipboardSettings::changeEncryptionPassword() { m_configuration->changeEncryptionPassword(); }
