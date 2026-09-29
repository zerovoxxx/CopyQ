// SPDX-License-Identifier: GPL-3.0-or-later

#include "gui/actionhandlerdialog.h"
#include "ui_actionhandlerdialog.h"

#include "common/actionhandlerenums.h"
#include "common/settings.h"
#include "gui/actionhandler.h"
#include "gui/theme.h"

#include <QSortFilterProxyModel>
#include <QSet>
#include <QLabel>
#include <QVBoxLayout>

namespace {

void terminateSelectedActions(QItemSelectionModel *selectionModel, ActionHandler *actionHandler)
{
     QSet<int> ids;
     for ( const auto &index : selectionModel->selectedIndexes() ) {
         const int actionId = index.data(ActionHandlerRole::id).toInt();
         ids.insert(actionId);
     }
     for (const int id : ids)
         actionHandler->terminateAction(id);
}

void updateTerminateButton(QItemSelectionModel *selectionModel, QAbstractItemModel *model, QPushButton *button)
{
     for ( const auto &index : selectionModel->selectedIndexes() ) {
         const int row = index.row();
         const auto statusIndex = model->index(row, 0);
         const auto state = static_cast<ActionState>(statusIndex.data(ActionHandlerRole::status).toInt());
         if (state == ActionState::Running || state == ActionState::Starting) {
             button->setEnabled(true);
             return;
         }
     }

     button->setEnabled(false);
}

} // namespace

ActionHandlerDialog::ActionHandlerDialog(ActionHandler *actionHandler, QAbstractItemModel *model, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ActionHandlerDialog)
{
    ui->setupUi(this);
    ui->horizontalLayout->removeWidget(ui->filterLineEdit);
    ui->horizontalLayout->removeWidget(ui->terminateButton);
    ui->verticalLayout->removeItem(ui->horizontalLayout);
    ui->verticalLayout->removeWidget(ui->tableView);
    auto controls = new QVBoxLayout;
    controls->addWidget(new QLabel(tr("Find a process"), this));
    controls->addWidget(ui->filterLineEdit);
    controls->addWidget(ui->terminateButton);
    controls->addStretch();
    auto body = new QHBoxLayout;
    body->addLayout(controls);
    body->addWidget(ui->tableView, 1);
    ui->verticalLayout->insertLayout(0, body, 1);
    Settings settings;
    Theme(settings).decorateDialog(this, tr("Running commands"), tr("Inspect command status and output. Terminate only the selected running commands."));
    resize(1000, 620);

    auto proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortRole(ActionHandlerRole::sort);
    proxyModel->setFilterKeyColumn(ActionHandlerColumn::name);
    ui->tableView->setModel(proxyModel);

    ui->tableView->resizeColumnsToContents();

    ui->tableView->sortByColumn(ActionHandlerColumn::status, Qt::DescendingOrder);

    connect( ui->filterLineEdit, &QLineEdit::textChanged, proxyModel,
             [proxyModel](const QString &pattern) {
                 const QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
                 proxyModel->setFilterRegularExpression(re);
             } );

    const auto selectionModel = ui->tableView->selectionModel();
    connect( ui->terminateButton, &QPushButton::clicked, this,
             [selectionModel, actionHandler]() {
                 terminateSelectedActions(selectionModel, actionHandler);
             } );

    const auto updateTerminateButtonSlot =
        [this, selectionModel, proxyModel]() {
            updateTerminateButton(selectionModel, proxyModel, ui->terminateButton);
        };

    connect( model, &QAbstractItemModel::dataChanged, this, updateTerminateButtonSlot );
    connect( selectionModel, &QItemSelectionModel::selectionChanged, this, updateTerminateButtonSlot );
}

ActionHandlerDialog::~ActionHandlerDialog()
{
    delete ui;
}
