// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "common/encryption.h"
#include <QObject>
#include <QVariantMap>
#include <QVariantList>

class SnippetStore final : public QObject
{
    Q_OBJECT
public:
    explicit SnippetStore(const QString &path, QObject *parent = nullptr);
    bool load(const Encryption::EncryptionKey &key = {});
    bool save(const Encryption::EncryptionKey &key = {});
    bool setDocument(const QVariantMap &document);
    QVariantMap document() const { return m_document; }
    QVariantList collections() const;
    QVariantList snippets() const;
    QVariantMap settings() const;
    QVariantMap snippet(const QString &id) const;
    QString effectiveKeyword(const QVariantMap &snippet) const;
    QString keywordError(const QString &id) const;
    QString createCollection(const QString &name);
    QString createSnippet(const QString &collection, const QVariantMap &data);
    bool updateCollection(const QString &id, const QVariantMap &fields);
    bool updateSnippet(const QString &id, const QVariantMap &fields);
    bool removeCollection(const QString &id);
    bool removeSnippet(const QString &id);
    void setSettings(const QVariantMap &settings);
    QByteArray exportCollection(const QString &id) const;
    bool importCollection(const QByteArray &bytes);
    QByteArray exportData() const;
    bool importData(const QByteArray &bytes);
    QString error() const { return m_error; }
    bool writable() const { return m_writable; }
    static bool validate(const QVariantMap &document, QString *error);

signals:
    void changed();

private:
    bool update(const char *list, const QString &id, const QVariantMap &fields, bool remove);
    QString m_path;
    QVariantMap m_document;
    QString m_error;
    bool m_writable = true;
};
