// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "common/encryption.h"
#include <QString>
#include <QByteArray>
#include <QVariantMap>
QString copyQConfigurationPath(const QString &directory);
bool prepareCopyQImport(const QString &directory, QByteArray *archive, QString *error,
                        const Encryption::EncryptionKey &key = {});
bool validateCopyQImportFiles(const QVariantMap &files, QString *error);
bool saveCopyQImportFiles(const QVariantMap &files, const QString &directory, QString *error);
bool readCopyQImportFiles(const QString &directory, QVariantMap *files, QString *error);
QVariant relocateCopyQImportPaths(const QVariant &value, const QString &from, const QString &to);
