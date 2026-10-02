// SPDX-License-Identifier: GPL-3.0-or-later
#include "copyqimport.h"
#include "item/clipboardmodel.h"
#include "item/serialize.h"
#include "common/contenttype.h"
#include <QDataStream>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QSaveFile>
#include <QRegularExpression>
#include <QUuid>

QVariant relocateCopyQImportPaths(const QVariant &value, const QString &from, const QString &to)
{
    if (value.metaType().id() == QMetaType::QString) { auto text = value.toString(); return text.replace(from, to); }
    if (value.metaType().id() == QMetaType::QStringList) {
        auto list = value.toStringList(); for (auto &text : list) text.replace(from, to); return list;
    }
    if (value.metaType().id() == QMetaType::QVariantMap) {
        auto map = value.toMap(); for (auto it = map.begin(); it != map.end(); ++it) it.value() = relocateCopyQImportPaths(it.value(), from, to); return map;
    }
    if (value.metaType().id() == QMetaType::QVariantList) {
        auto list = value.toList(); for (auto &item : list) item = relocateCopyQImportPaths(item, from, to); return list;
    }
    return value;
}

bool validateCopyQImportFiles(const QVariantMap &files, QString *error)
{
    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        const auto path = it.key();
        if (path.isEmpty() || QDir::isAbsolutePath(path) || path.contains(QLatin1Char('\\'))
                || path.contains(QLatin1Char(':')) || path.split(QLatin1Char('/')).contains(QStringLiteral(".."))
                || QDir::cleanPath(path) != path || it.value().metaType().id() != QMetaType::QByteArray) {
            *error = QObject::tr("Invalid relative CopyQ import asset: %1").arg(path); return false;
        }
    }
    return true;
}

bool readCopyQImportFiles(const QString &directory, QVariantMap *files, QString *error)
{
    files->clear();
    QDir root(directory);
    if (!root.exists()) return true;
    const auto canonical = QFileInfo(directory).canonicalFilePath();
    QDirIterator entries(directory, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    qint64 size = 0;
    while (entries.hasNext()) {
        const auto path = entries.next();
        QFile file(path);
        const auto info = entries.fileInfo();
        if (info.isSymLink() || !info.canonicalFilePath().startsWith(canonical + QLatin1Char('/'))
                || !file.open(QIODevice::ReadOnly) || (size += file.size()) > 1024LL*1024*1024) {
            *error = QObject::tr("Unreadable, linked or oversized CopyQ profile asset: %1").arg(path); files->clear(); return false;
        }
        files->insert(root.relativeFilePath(path), file.readAll());
        if (file.error() != QFile::NoError) { *error = file.errorString(); files->clear(); return false; }
    }
    return validateCopyQImportFiles(*files, error);
}

bool saveCopyQImportFiles(const QVariantMap &files, const QString &directory, QString *error)
{
    if (!validateCopyQImportFiles(files, error)) return false;
    QDir root(directory);
    if (QFileInfo(directory).isSymLink()) { *error = QObject::tr("Linked import asset directories are not allowed."); return false; }
    if (!files.isEmpty() && !root.mkpath(QStringLiteral("."))) { *error = QObject::tr("Unable to create import asset directory."); return false; }
    const auto canonical = QFileInfo(directory).canonicalFilePath();
    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        const auto path = root.filePath(it.key());
        const auto parent = QFileInfo(path).absolutePath();
        if (!root.mkpath(QFileInfo(it.key()).path()) || QFileInfo(path).isSymLink()
                || !(QFileInfo(parent).canonicalFilePath() == canonical || QFileInfo(parent).canonicalFilePath().startsWith(canonical + QLatin1Char('/')))) {
            *error = QObject::tr("Unsafe import asset destination: %1").arg(path); return false;
        }
        QSaveFile file(path); file.setDirectWriteFallback(false);
        const auto bytes = it.value().toByteArray();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) { *error = file.errorString(); return false; }
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
    return true;
}

QString copyQConfigurationPath(const QString &directory)
{
    QString selected;
    QDirIterator files(directory, {QStringLiteral("copyq.ini"), QStringLiteral("copyq-*.ini")}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const auto file = files.next();
        if (QFileInfo(file).isSymLink()) continue;
        QSettings candidate(file, QSettings::IniFormat);
        if (!candidate.contains(QStringLiteral("Options/tabs"))) continue;
        if (!selected.isEmpty()) return {}; // Ambiguous profiles require a narrower directory.
        selected = file;
    }
    return selected;
}

bool prepareCopyQImport(const QString &directory, QByteArray *archive, QString *error, const Encryption::EncryptionKey &key)
{
    archive->clear(); error->clear();
    const auto fail = [error](const QString &message) { *error = message; return false; };
    const auto configPath = copyQConfigurationPath(directory);
    if (configPath.isEmpty()) return fail(QObject::tr("Choose a profile containing exactly one copyq[-session].ini."));
    QSettings config(configPath, QSettings::IniFormat);
    const auto oldName = QFileInfo(configPath).completeBaseName();
    const auto tabs = config.value(QStringLiteral("Options/tabs")).toStringList();
    QVariantMap settings;
    const auto sourceRoot = QFileInfo(directory).canonicalFilePath();
    if (sourceRoot.isEmpty()) return fail(QObject::tr("The source profile directory is unavailable."));
    for (const auto &name : config.allKeys()) {
        // Keep the receiving application's startup and encryption identity.
        if (name == QLatin1String("Options/autostart") || name == QLatin1String("Options/encrypt_tabs") || name == QLatin1String("Options/use_key_store")) continue;
        settings.insert(name, config.value(name));
    }
    if (config.status() != QSettings::NoError) return fail(QObject::tr("Unable to read CopyQ configuration."));
    QVariantList commands;
    QSettings commandConfig(QFileInfo(configPath).absolutePath() + QLatin1Char('/') + oldName + QStringLiteral("-commands.ini"), QSettings::IniFormat);
    const int count = commandConfig.beginReadArray(QStringLiteral("Commands"));
    for (int i = 0; i < count; ++i) {
        commandConfig.setArrayIndex(i);
        QVariantMap command;
        for (const auto &name : commandConfig.allKeys()) command.insert(name, commandConfig.value(name));
        commands.append(command);
    }
    commandConfig.endArray();
    commandConfig.beginGroup(QStringLiteral("Command"));
    QVariantMap single;
    for (const auto &name : commandConfig.allKeys()) single.insert(name, commandConfig.value(name));
    if (!single.isEmpty()) commands.append(single);
    commandConfig.endGroup();
    if (commandConfig.status() != QSettings::NoError) return fail(QObject::tr("Unable to read CopyQ commands."));
    QVariantMap assets;
    if (!readCopyQImportFiles(directory, &assets, error)) return false;
    QHash<QString, QString> sourceFiles;
    const QRegularExpression dataPath(QStringLiteral("(?:^|/)((?:[a-f0-9]{16}/){3}[a-f0-9]{16}\\.dat)$"));
    for (auto it = assets.cbegin(); it != assets.cend(); ++it) {
        const auto match = dataPath.match(it.key());
        if (!match.hasMatch()) continue;
        const auto relative = match.captured(1);
        if (sourceFiles.contains(relative)) return fail(QObject::tr("Ambiguous CopyQ data file: %1").arg(relative));
        sourceFiles.insert(relative, QDir(sourceRoot).filePath(it.key()));
    }
    const auto identity = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVariantMap files;
    for (auto it = assets.cbegin(); it != assets.cend(); ++it) files.insert(identity + QLatin1Char('/') + it.key(), it.value());
    const auto portableRoot = QStringLiteral("qclip-import://") + identity + QLatin1Char('/');
    // macOS exposes /tmp as a symlink to /private/tmp; settings can use either spelling.
    for (const auto &root : {QDir(directory).absolutePath(), sourceRoot}) {
        settings = relocateCopyQImportPaths(settings, root + QLatin1Char('/'), portableRoot).toMap();
        commands = relocateCopyQImportPaths(commands, root + QLatin1Char('/'), portableRoot).toList();
    }
    QByteArray result;
    QDataStream output(&result, QIODevice::WriteOnly);
    output.setVersion(QDataStream::Qt_4_7);
    output << QByteArray("CopyQ v4") << QVariantMap{{QStringLiteral("tabs"), tabs}, {QStringLiteral("settings"), settings}, {QStringLiteral("commands"), commands}, {QStringLiteral("files"), files}};
    for (const auto &tab : tabs) {
        auto part = QString::fromUtf8(tab.toUtf8().toBase64()); part.replace(QLatin1Char('/'), QLatin1Char('-'));
        const auto name = oldName + QStringLiteral("_tab_") + part + QStringLiteral(".dat");
        QDirIterator files(directory, {name}, QDir::Files, QDirIterator::Subdirectories);
        QString path;
        while (files.hasNext()) { if (!path.isEmpty()) return fail(QObject::tr("Duplicate CopyQ tab files: %1").arg(tab)); path = files.next(); }
        if (path.isEmpty()) return fail(QObject::tr("Missing CopyQ history: %1").arg(tab));
        QFile file(path);
        ClipboardModel model;
        if (!file.open(QIODevice::ReadOnly) || !deserializeData(&model, &file, &key)) return fail(QObject::tr("Unable to read/decrypt CopyQ collection: %1").arg(tab));
        // Resolve every external file only inside the explicitly selected source profile.
        for (int row = 0; row < model.rowCount(); ++row) {
            auto data = model.index(row, 0).data(contentType::data).toMap();
            if (!materializeData(&data, sourceRoot, sourceFiles, error)) return false;
            model.setData(model.index(row, 0), data, contentType::data);
        }
        QByteArray bytes;
        QDataStream tabOutput(&bytes, QIODevice::WriteOnly);
        tabOutput.setVersion(QDataStream::Qt_4_7);
        if (!serializeData(model, &tabOutput)) return fail(QObject::tr("Unable to encode migrated history."));
        output << QVariantMap{{QStringLiteral("name"), tab}, {QStringLiteral("data"), bytes}};
    }
    if (output.status() != QDataStream::Ok) return fail(QObject::tr("Unable to encode migrated profile."));
    *archive = result;
    return true;
}
