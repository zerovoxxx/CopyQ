// SPDX-License-Identifier: GPL-3.0-or-later
#include "snippetstore.h"
#include "item/serialize.h"
#include "common/mimetypes.h"
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <QUuid>

namespace {
const QString payload = QStringLiteral("application/x-qclip-snippets");
constexpr qint64 maxSnippetStorageBytes = 100'000'000;
QVariantMap emptyDocument()
{
    return {{QStringLiteral("version"), 1}, {QStringLiteral("collections"), QVariantList()},
            {QStringLiteral("snippets"), QVariantList()}, {QStringLiteral("settings"), QVariantMap()}};
}
QByteArray encode(const QVariantMap &document)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << document;
    return bytes;
}
bool decode(const QByteArray &bytes, QVariantMap *document)
{
    if (bytes.isEmpty() || bytes.size() > maxSnippetStorageBytes)
        return false;
    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_6_0);
    stream >> *document;
    return stream.status() == QDataStream::Ok && stream.atEnd();
}
QString identity() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
}

SnippetStore::SnippetStore(const QString &path, QObject *parent)
    : QObject(parent), m_path(path), m_document(emptyDocument()) {}

bool SnippetStore::validate(const QVariantMap &document, QString *error)
{
    const auto fail = [error](const QString &message) { *error = message; return false; };
    if ((document.value(QStringLiteral("version")).metaType().id() != QMetaType::Int || document.value(QStringLiteral("version")).toInt() != 1))
        return fail(tr("Unsupported Snippet data version."));
    for (const auto field : {"collections", "snippets"})
        if (document.value(QLatin1String(field)).metaType().id() != QMetaType::QVariantList)
            return fail(tr("Invalid Snippet list."));
    QSet<QString> ids;
    QSet<QString> collections;
    for (const auto &value : document.value(QStringLiteral("collections")).toList()) {
        const auto collection = value.toMap();
        const auto id = collection.value(QStringLiteral("id")).toString();
        if (id.isEmpty() || ids.contains(id) || collection.value(QStringLiteral("name")).toString().trimmed().isEmpty())
            return fail(tr("Missing or duplicate collection identity/name."));
        ids.insert(id);
        collections.insert(id);
    }
    for (const auto &value : document.value(QStringLiteral("snippets")).toList()) {
        const auto snippet = value.toMap();
        const auto id = snippet.value(QStringLiteral("id")).toString();
        if (id.isEmpty() || ids.contains(id) || !collections.contains(snippet.value(QStringLiteral("collection")).toString()))
            return fail(tr("Missing collection or duplicate Snippet identity."));
        if (snippet.value(QStringLiteral("title")).toString().trimmed().isEmpty()
                || snippet.value(QStringLiteral("data")).metaType().id() != QMetaType::QVariantMap)
            return fail(tr("A Snippet needs a title and MIME data."));
        const auto keyword = snippet.value(QStringLiteral("keyword")).toString();
        if (keyword.size() > 512 || keyword.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f]"))))
            return fail(tr("Keywords must fit 512 UTF-16 units and contain no control characters."));
        ids.insert(id);
    }
    for (const auto &value : document.value(QStringLiteral("collections")).toList()) {
        const auto collection = value.toMap();
        const auto affixes = collection.value(QStringLiteral("prefix")).toString() + collection.value(QStringLiteral("suffix")).toString();
        if (affixes.size() > 256 || affixes.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f]"))))
            return fail(tr("Collection affixes must fit 256 UTF-16 units and contain no control characters."));
    }
    error->clear();
    return true;
}

bool SnippetStore::setDocument(const QVariantMap &document)
{
    if (!validate(document, &m_error))
        return false;
    if (!m_writable) {
        m_error = tr("Snippet storage is locked or damaged; the original file was preserved.");
        return false;
    }
    m_document = document;
    emit changed();
    return true;
}

bool SnippetStore::load(const Encryption::EncryptionKey &key)
{
    if (!QFile::exists(m_path))
        return true;
    QFile file(m_path);
    QVariantMap data;
    QVariantMap document;
    m_writable = false;
    m_document = emptyDocument();
    if (!file.open(QIODevice::ReadOnly) || file.size() > maxSnippetStorageBytes) {
        m_error = tr("Unable to read Snippet storage.");
        return false;
    }
    QDataStream stream(&file);
    if (!deserializeData(&stream, &data, &key) || !stream.atEnd()
            || !decode(data.value(payload).toByteArray(), &document) || !validate(document, &m_error)) {
        if (m_error.isEmpty()) m_error = tr("Unable to decode or unlock Snippet storage.");
        return false;
    }
    m_writable = true;
    return setDocument(document);
}

bool SnippetStore::save(const Encryption::EncryptionKey &key)
{
    if (!m_writable || !validate(m_document, &m_error))
        return false;
    const auto bytes=encode(m_document);
    if (bytes.size()>maxSnippetStorageBytes) {
        m_error=tr("Snippet storage exceeds the 100 MB limit; the previous file was preserved.");
        return false;
    }
    if (!QDir().mkpath(QFileInfo(m_path).absolutePath())) {
        m_error = tr("Unable to create Snippet storage directory.");
        return false;
    }
    QFile::setPermissions(QFileInfo(m_path).absolutePath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QSaveFile file(m_path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) { m_error = file.errorString(); return false; }
    QDataStream stream(&file);
    serializeData(&stream, {{payload, bytes}}, -1, &key);
    if (stream.status() != QDataStream::Ok || file.size()>maxSnippetStorageBytes || !file.commit()) {
        m_error = tr("Unable to save Snippets; the previous file was preserved.");
        return false;
    }
    m_error.clear();
    QFile::setPermissions(m_path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

QVariantList SnippetStore::collections() const { return m_document.value(QStringLiteral("collections")).toList(); }
QVariantList SnippetStore::snippets() const { return m_document.value(QStringLiteral("snippets")).toList(); }
QVariantMap SnippetStore::settings() const { return m_document.value(QStringLiteral("settings")).toMap(); }
QVariantMap SnippetStore::snippet(const QString &id) const
{
    for (const auto &value : snippets())
        if (value.toMap().value(QStringLiteral("id")).toString() == id) return value.toMap();
    return {};
}
QString SnippetStore::effectiveKeyword(const QVariantMap &snippet) const
{
    const auto keyword = snippet.value(QStringLiteral("keyword")).toString();
    if (keyword.isEmpty()) return {};
    for (const auto &value : collections()) {
        const auto collection = value.toMap();
        if (collection.value(QStringLiteral("id")) == snippet.value(QStringLiteral("collection")))
            return collection.value(QStringLiteral("prefix")).toString() + keyword + collection.value(QStringLiteral("suffix")).toString();
    }
    return {};
}
QString SnippetStore::keywordError(const QString &id) const
{
    const auto keyword = effectiveKeyword(snippet(id));
    if (keyword.isEmpty()) return {};
    int count = 0;
    for (const auto &value : snippets()) if (effectiveKeyword(value.toMap()) == keyword) ++count;
    return count > 1 ? tr("Keyword conflict: %1").arg(keyword) : QString();
}
QString SnippetStore::createCollection(const QString &name)
{
    const auto id = identity();
    auto document = m_document;
    auto list = collections();
    list.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("name"), name},
        {QStringLiteral("enabled"), true}, {QStringLiteral("prefix"), QString()}, {QStringLiteral("suffix"), QString()}});
    document.insert(QStringLiteral("collections"), list);
    return setDocument(document) ? id : QString();
}
QString SnippetStore::createSnippet(const QString &collection, const QVariantMap &data)
{
    const auto id = identity();
    QVariantMap bytes;
    // Materialize every MIME before the history can release its external DataFiles.
    for (auto it = data.cbegin(); it != data.cend(); ++it)
        bytes.insert(it.key(), it.value().toByteArray());
    auto document = m_document;
    auto list = snippets();
    list.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("collection"), collection},
        {QStringLiteral("title"), tr("New Snippet")}, {QStringLiteral("keyword"), QString()},
        {QStringLiteral("enabled"), false}, {QStringLiteral("data"), bytes}});
    document.insert(QStringLiteral("snippets"), list);
    return setDocument(document) ? id : QString();
}
bool SnippetStore::update(const char *listName, const QString &id, const QVariantMap &fields, bool remove)
{
    auto document = m_document;
    auto list = document.value(QLatin1String(listName)).toList();
    for (qsizetype i = 0; i < list.size(); ++i) {
        auto value = list.at(i).toMap();
        if (value.value(QStringLiteral("id")).toString() != id) continue;
        if (remove) list.removeAt(i);
        else {
            for (auto it = fields.cbegin(); it != fields.cend(); ++it)
                if (it.key() != QLatin1String("id")) value.insert(it.key(), it.value());
            list[i] = value;
        }
        document.insert(QLatin1String(listName), list);
        return setDocument(document);
    }
    m_error = tr("The Snippet or collection no longer exists.");
    return false;
}
bool SnippetStore::updateCollection(const QString &id, const QVariantMap &fields) { return update("collections", id, fields, false); }
bool SnippetStore::updateSnippet(const QString &id, const QVariantMap &fields) { return update("snippets", id, fields, false); }
bool SnippetStore::removeSnippet(const QString &id) { return update("snippets", id, {}, true); }
bool SnippetStore::removeCollection(const QString &id)
{
    auto original = m_document;
    auto list = snippets();
    for (qsizetype i = list.size(); i-- > 0;)
        if (list.at(i).toMap().value(QStringLiteral("collection")).toString() == id) list.removeAt(i);
    m_document.insert(QStringLiteral("snippets"), list);
    if (update("collections", id, {}, true)) return true;
    m_document = original;
    return false;
}
void SnippetStore::setSettings(const QVariantMap &settings)
{
    auto document = m_document;
    document.insert(QStringLiteral("settings"), settings);
    setDocument(document);
}
QByteArray SnippetStore::exportData() const { return encode(m_document); }
bool SnippetStore::importData(const QByteArray &bytes)
{
    QVariantMap document;
    if (!decode(bytes, &document)) { m_error = tr("Invalid Snippet data."); return false; }
    return setDocument(document);
}
QByteArray SnippetStore::exportCollection(const QString &id) const
{
    auto document = emptyDocument();
    QVariantList selectedCollections, selectedSnippets;
    for (const auto &value : collections()) if (value.toMap().value(QStringLiteral("id")).toString() == id) selectedCollections.append(value);
    for (const auto &value : snippets()) if (value.toMap().value(QStringLiteral("collection")).toString() == id) selectedSnippets.append(value);
    if (selectedCollections.isEmpty()) return {};
    document.insert(QStringLiteral("collections"), selectedCollections);
    document.insert(QStringLiteral("snippets"), selectedSnippets);
    return encode(document);
}
bool SnippetStore::importCollection(const QByteArray &bytes)
{
    QVariantMap imported;
    if (!decode(bytes, &imported) || !validate(imported, &m_error)) {
        if (m_error.isEmpty()) m_error = tr("Invalid Snippet collection.");
        return false;
    }
    auto document = m_document;
    for (const auto field : {"collections", "snippets"}) {
        auto list = document.value(QLatin1String(field)).toList();
        list.append(imported.value(QLatin1String(field)).toList());
        document.insert(QLatin1String(field), list);
    }
    return setDocument(document);
}
