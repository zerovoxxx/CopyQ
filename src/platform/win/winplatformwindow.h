// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


#include "platform/platformwindow.h"

#ifndef WINVER
#define WINVER 0x0500
#endif
#include <qt_windows.h>

class AppConfig;

class WinPlatformWindow final : public PlatformWindow
{
public:
    explicit WinPlatformWindow(HWND window);

    QString getTitle() override;
    QString getApplicationId() const override;

    void raise() override;

    bool pasteFromClipboard() override;
    bool copyToClipboard() override;

    bool matchesWidget(const QWidget *widget) const override;
    bool matchesWindow(const QWindow *window) const override;
    bool isValid() const override;
    bool isActive() const override;
    bool pasteFromClipboardSafely(const std::function<bool()> &canPaste) override;

private:
    bool sendKeyPress(WORD modifier, WORD key, const AppConfig &config, bool requireFocus = false,
                      const std::function<bool()> &canPaste = {});

    HWND m_window;
    DWORD m_processId = 0;
};
