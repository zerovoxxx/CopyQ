// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "gui/clipboardbrowsershared.h"
#include "common/snippets.h"
#include "platform/platformnativeinterface.h"
#include "gui/clipboardwindow.h"
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
class SnippetStore;

class ClipboardSnippets final : public ClipboardWindow
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ClipboardSnippetsWindow)
    QML_UNCREATABLE("Owned by MainWindow")
    Q_PROPERTY(QVariantList collections READ collections NOTIFY changed)
    Q_PROPERTY(QVariantList snippets READ snippets NOTIFY changed)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY changed)
    Q_PROPERTY(QVariantMap theme READ theme CONSTANT)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString collectionId READ collectionId WRITE setCollectionId NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString inputStatus READ inputStatus NOTIFY changed)
    Q_PROPERTY(QString preview READ preview NOTIFY changed)
    Q_PROPERTY(QString richPreview READ richPreview NOTIFY changed)
public:
    ClipboardSnippets(SnippetStore *store, const ClipboardBrowserSharedPtr &shared);
    ~ClipboardSnippets() override;
    void open(const PlatformWindowPtr &target);
    QVariantList collections() const;
    QVariantList snippets() const;
    QVariantMap selected() const;
    QVariantMap settings() const;
    QVariantMap theme() const;
    QString query() const { return m_query; }
    QString collectionId() const { return m_collection; }
    QString error() const { return m_error; }
    QString inputStatus() const { return m_inputStatus; }
    QString preview() const;
    QString richPreview() const;
    void setInputStatus(const QString &status) { m_inputStatus = status; emit changed(); }
    void setQuery(const QString &query) { m_query = query; emit changed(); }
    void setCollectionId(const QString &id) { m_collection = id; m_selected.clear(); emit changed(); }
    void setContext(const std::function<SnippetContext()> &context) { m_context = context; }
    Q_INVOKABLE void select(const QString &id);
    Q_INVOKABLE QString createCollection(const QString &name);
    Q_INVOKABLE bool editCollection(const QString &id, const QVariantMap &fields);
    Q_INVOKABLE bool removeCollection(const QString &id);
    Q_INVOKABLE QString createSnippet();
    Q_INVOKABLE bool editSnippet(const QVariantMap &fields);
    Q_INVOKABLE bool removeSnippet();
    Q_INVOKABLE void editBody();
    Q_INVOKABLE void useSnippet(bool paste);
    Q_INVOKABLE void runCommand();
    Q_INVOKABLE void setSetting(const QString &name, const QVariant &value);
    Q_INVOKABLE void importFile();
    Q_INVOKABLE void exportFile();
    Q_INVOKABLE void importCopyQ();
signals:
    void snippetSaved();
    void changed();
    void outputRequested(const QVariantMap &data, int cursor, bool paste, const PlatformWindowPtr &target);
    void copyQImportRequested(const QString &directory);
    void commandRequested(const QString &name, const QVariantMap &data, const PlatformWindowPtr &target);
private:
    bool commit(const QVariantMap &before, bool success);
    SnippetResult render() const;
    SnippetStore *m_store;
    ClipboardBrowserSharedPtr m_shared;
    PlatformWindowPtr m_target;
    std::function<SnippetContext()> m_context;
    QString m_selected;
    QString m_collection;
    QString m_query;
    QString m_error;
    QString m_inputStatus;
};
