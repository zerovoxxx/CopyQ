// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "item/itemfilter.h"
#include "item/persistentdisplayitem.h"

#include <QAbstractListModel>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QQuickImageProvider>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class ItemFactory;
class ClipboardPaletteModel;

class PaletteImageProvider final : public QQuickImageProvider
{
public:
    explicit PaletteImageProvider(ClipboardPaletteModel *model);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QPointer<ClipboardPaletteModel> m_model;
};

// Owns presentation and persistent indexes only. ClipboardBrowser owns the history.
class ClipboardPaletteModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ClipboardPalette")
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int count READ count NOTIFY resultsChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY resultsChanged)
    Q_PROPERTY(bool filtering READ filtering NOTIFY resultsChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE selectRow NOTIFY selectionChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap preview READ preview NOTIFY previewChanged)

public:
    enum Role { SummaryRole = Qt::UserRole + 1, TypeRole, SelectedRole, NotesRole, TagsRole, PinnedRole, SourceRowRole };
    Q_ENUM(Role)

    explicit ClipboardPaletteModel(ItemFactory *factory, QObject *parent = nullptr);
    void setSourceModel(QAbstractItemModel *source, const QString &tabName);
    QAbstractItemModel *sourceModel() const { return m_source; }
    ItemFactory *itemFactory() const;
    const QString &tabName() const { return m_tabName; }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return rowCount(); }
    int sourceCount() const;
    bool filtering() const { return m_timer.isActive(); }
    QString query() const { return m_query; }
    void setQuery(const QString &query);
    int selectedRow() const;
    Q_INVOKABLE void selectRow(int row);
    Q_INVOKABLE void selectNext(int step);
    Q_INVOKABLE void select(int row, bool extend = false, bool toggle = false);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE bool isSelected(int row) const;
    int selectedCount() const { return m_selection.size(); }
    QList<QPersistentModelIndex> selectedIndexes() const;
    void setSelectedIndexes(const QList<QPersistentModelIndex> &indexes);
    QPersistentModelIndex sourceIndex(int row) const;
    int resultRow(const QModelIndex &index) const;
    QPersistentModelIndex selectedIndex() const { return m_selected; }
    QVariantMap selectedData() const;
    QVariantMap preview() const;
    QVariantMap displayData() const;
    int displayRevision() const { return m_displayRevision; }
    Q_INVOKABLE void requestDisplay(int row);
    void setDisplayEnabled(bool enabled);
    bool isDisplayDataValid(const QPersistentModelIndex &index, int revision) const;
    void setDisplayData(const QPersistentModelIndex &index, int revision, const QVariantMap &data);

signals:
    void queryChanged();
    void resultsChanged();
    void selectionChanged();
    void previewChanged();
    void selectionInvalidated();
    void itemDisplayRequested(const PersistentDisplayItem &item);
    void displaysInvalidated();

private:
    void rebuild(bool selectFirst);
    void filterBatch();
    void updatePreview();

    QPointer<ItemFactory> m_factory;
    QPointer<QAbstractItemModel> m_source;
    QList<QMetaObject::Connection> m_sourceConnections;
    QList<QPersistentModelIndex> m_results;
    QPersistentModelIndex m_selected;
    QPersistentModelIndex m_anchor;
    QList<QPersistentModelIndex> m_selection;
    QVariantMap m_displayData;
    struct DisplayData {
        QPersistentModelIndex index;
        int revision;
        QVariantMap data;
    };
    QList<DisplayData> m_displayItems;
    ItemFilterPtr m_filter;
    QString m_query;
    QString m_tabName;
    QTimer m_timer;
    int m_nextRow = 0;
    int m_displayRevision = 0;
    int m_nextDisplayRevision = 0;
    int m_previewRevision = 0;
    bool m_selectFirst = true;
    bool m_displayEnabled = true;
};
