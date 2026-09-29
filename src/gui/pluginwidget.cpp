// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginwidget.h"
#include "ui_pluginwidget.h"

#include "item/itemwidget.h"

#include <QBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QSplitter>

PluginWidget::PluginWidget(const ItemLoaderPtr &loader, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PluginWidget)
    , m_loader(loader)
{
    ui->setupUi(this);

    const QString author = m_loader->author();
    if (author.isEmpty())
        ui->labelAuthor->hide();
    else
        ui->labelAuthor->setText(author);

    const QString description = m_loader->description();
    if (description.isEmpty())
        ui->labelDescription->hide();
    else
        ui->labelDescription->setText(m_loader->description());

    QWidget *loaderSettings = m_loader->createSettingsWidget(this);
    if (loaderSettings) {
        for (auto form : loaderSettings->findChildren<QFormLayout *>()) {
            form->setRowWrapPolicy(QFormLayout::WrapAllRows);
            form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
            form->setSpacing(10);
        }
        for (auto box : loaderSettings->findChildren<QBoxLayout *>()) {
            box->setSpacing(12);
            if (box->direction() == QBoxLayout::LeftToRight && box->count() == 2
                    && qobject_cast<QLabel *>(box->itemAt(0)->widget()))
                box->setDirection(QBoxLayout::TopToBottom);
        }
        for (auto splitter : loaderSettings->findChildren<QSplitter *>())
            splitter->setOrientation(Qt::Vertical);
        auto scroll = new QScrollArea(this);
        scroll->setObjectName(QStringLiteral("plugin_settings_form"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(loaderSettings);
        ui->verticalLayout->insertWidget(2, scroll);
        ui->verticalLayout->setStretch(2, 1);
    }
    ui->verticalLayout->setSpacing(16);
    ui->labelDescription->setWordWrap(true);
    ui->labelAuthor->setWordWrap(true);
}

PluginWidget::~PluginWidget()
{
    delete ui;
}
