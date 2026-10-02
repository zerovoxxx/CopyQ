// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QtContainerFwd>

class QAbstractItemModel;
class QDataStream;
class QIODevice;
class QString;

namespace Encryption {
    class EncryptionKey;
}

class DataFile;
QDataStream &operator<<(QDataStream &out, DataFile value);
QDataStream &operator>>(QDataStream &in, DataFile &value);
void registerDataFileConverter();

void serializeData(QDataStream *stream, const QVariantMap &data, int itemDataThreshold = -1, const Encryption::EncryptionKey *encryptionKey = nullptr);
bool deserializeData(QDataStream *stream, QVariantMap *data, const Encryption::EncryptionKey *encryptionKey = nullptr);
QByteArray serializeData(const QVariantMap &data);
bool deserializeData(QVariantMap *data, const QByteArray &bytes, const Encryption::EncryptionKey *encryptionKey = nullptr);

bool serializeData(const QAbstractItemModel &model, QDataStream *stream, int itemDataThreshold = -1, const Encryption::EncryptionKey *encryptionKey = nullptr);
bool deserializeData(QAbstractItemModel *model, QDataStream *stream, const Encryption::EncryptionKey *encryptionKey = nullptr);
bool serializeData(const QAbstractItemModel &model, QIODevice *file, int itemDataThreshold = -1, const Encryption::EncryptionKey *encryptionKey = nullptr);
bool deserializeData(QAbstractItemModel *model, QIODevice *file, const Encryption::EncryptionKey *encryptionKey = nullptr);

bool itemDataFiles(QIODevice *file, QStringList *files, const Encryption::EncryptionKey *encryptionKey = nullptr);

bool materializeData(QVariantMap *data, const QString &sourceRoot,
                     const QHash<QString, QString> &sourceFiles, QString *error);

qint64 estimateDataSize(const QVariantMap &data);
