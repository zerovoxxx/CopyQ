// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
class QWindow;
// Returns false when the compositor or accessibility settings require an opaque surface.
bool setWindowBlur(QWindow *window, bool dark);
