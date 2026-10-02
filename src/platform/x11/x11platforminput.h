// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// X11 device events have no injection tag; match only this process's queued keys.
void recordInjectedX11Key(unsigned int code, bool pressed);
