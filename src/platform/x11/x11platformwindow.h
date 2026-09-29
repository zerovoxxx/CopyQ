// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


#include "platform/platformwindow.h"

class AppConfig;
class QWidget;

class X11PlatformWindow final : public PlatformWindow
{
public:
    explicit X11PlatformWindow();

    explicit X11PlatformWindow(quintptr winId);

    QString getTitle() override;
    QString getApplicationId() const override;

    void raise() override;

    bool pasteFromClipboard() override;

    bool copyToClipboard() override;

    bool isValid() const override;
    bool isActive() const override;
    bool pasteFromClipboardSafely(const std::function<bool()> &canPaste) override;

    bool matchesWidget(const QWidget *widget) const override;
    bool matchesWindow(const QWindow *window) const override;

private:
    bool waitForFocus(int ms);

    bool sendKeyPress(int modifier, int key, const AppConfig &config, bool requireFocus = false,
                      const std::function<bool()> &canPaste = {});

    quintptr m_window;
};
