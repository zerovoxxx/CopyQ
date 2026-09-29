// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVariantList>

QVariantList backupClipboard();
void restoreClipboard(const QVariantList &items);
