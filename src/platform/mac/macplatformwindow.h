// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "objcstrong.h"
#include "cfref.h"
#include <ApplicationServices/ApplicationServices.h>

#include "platform/platformwindow.h"

// For WId
#include <QWidget>

@class NSWindow;
@class NSRunningApplication;

class MacPlatformWindow final : public PlatformWindow
{
public:
    MacPlatformWindow();
    explicit MacPlatformWindow(WId wid);
    explicit MacPlatformWindow(NSRunningApplication *runningApplication);

    virtual ~MacPlatformWindow();

    /**
     * Return window title text of current window.
     *
     * On OS X, this gets the title of the application instead of the window.
     */
    QString getTitle() override;

    void raise() override;
    bool pasteFromClipboard() override;
    bool copyToClipboard() override;
    bool matchesWidget(const QWidget *widget) const override;
    bool matchesWindow(const QWindow *window) const override;
    bool isValid() const override;
    bool isActive() const override;
    bool pasteFromClipboardSafely(const std::function<bool()> &canPaste) override;

private:
    // Don't allow copies
    Q_DISABLE_COPY(MacPlatformWindow)

    long int m_windowNumber = -1;
    ObjCStrong<NSWindow> m_window;
    ObjCStrong<NSRunningApplication> m_runningApplication;
    CFRef<AXUIElementRef> m_accessibilityWindow;
};
