// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/platformcommon.h"
#include "winplatformwindow.h"

#include "common/appconfig.h"
#include "common/log.h"
#include "common/sleeptimer.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QWindow>
#include <iterator>

namespace {

QString windowTitle(HWND window)
{
    WCHAR buf[1024];
    GetWindowTextW(window, buf, 1024);
    return QString::fromUtf16(reinterpret_cast<ushort *>(buf));
}

INPUT createInput(WORD key, DWORD flags = 0)
{
    INPUT input{};

    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    input.ki.wScan = 0;
    input.ki.dwFlags = flags;
    input.ki.time = 0;
    input.ki.dwExtraInfo = GetMessageExtraInfo();

    return input;
}

QString windowLogText(QString text, HWND window)
{
    const QString windowInfo =
            QString("%1").arg(reinterpret_cast<quintptr>(window))
            + " \"" + windowTitle(window) + "\"";

    text.prepend("Window " + windowInfo + ": ");

    const DWORD lastError = GetLastError();
    if (lastError != 0)
        text.append( QString(" (last error is %1)").arg(GetLastError()) );

    return text;
}

void logWindowWarning(const char *text, HWND window)
{
    log( windowLogText(text, window), LogWarning );
}

void logWindowDebug(const char *text, HWND window)
{
    COPYQ_LOG( windowLogText(text, window) );
}

bool raiseWindowHelper(HWND window)
{
    if (!SetForegroundWindow(window)) {
        logWindowWarning("Failed to raise: SetForegroundWindow() == false", window);
        return false;
    }

    SetWindowPos(window, HWND_TOP, 0, 0, 0, 0,
                 SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    logWindowDebug("Raised", window);

    return true;
}

bool raiseWindow(HWND window)
{
    if (!IsWindowVisible(window)) {
        logWindowWarning("Failed to raise: IsWindowVisible() == false", window);
        return false;
    }

    // WORKAROUND: Set foreground window if even this process is not in foreground.
    const auto thisThreadId = GetCurrentThreadId();
    const auto foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    if (thisThreadId != foregroundThreadId) {
        if ( AttachThreadInput(thisThreadId, foregroundThreadId, true) ) {
            logWindowDebug("Attached foreground thread", window);
            const bool result = raiseWindowHelper(window);
            AttachThreadInput(thisThreadId, foregroundThreadId, false);
            return result;
        }

        logWindowDebug("Failed to attach foreground thread", window);
    }

    return raiseWindowHelper(window);
}

bool isKeyPressed(int key)
{
    return GetKeyState(key) & 0x8000;
}

bool isModifierPressed()
{
    return isKeyPressed(VK_LWIN)
        || isKeyPressed(VK_RWIN)
        || isKeyPressed(VK_LCONTROL)
        || isKeyPressed(VK_RCONTROL)
        || isKeyPressed(VK_LSHIFT)
        || isKeyPressed(VK_RSHIFT)
        || isKeyPressed(VK_LMENU)
        || isKeyPressed(VK_RMENU)
        || isKeyPressed(VK_MENU);
}

bool waitForModifiersReleased(const AppConfig &config)
{
    const int maxWaitForModsReleaseMs = config.option<Config::window_wait_for_modifier_released_ms>();
    if (maxWaitForModsReleaseMs >= 0) {
        SleepTimer t(maxWaitForModsReleaseMs);
        while (t.sleep()) {
            if (!isModifierPressed())
                return true;
        }
    }

    return !isModifierPressed();
}

bool sendInputs(QVector<INPUT> input, HWND wnd)
{
    const UINT numberOfAddedEvents = SendInput( input.size(), input.data(), sizeof(INPUT) );
    if (numberOfAddedEvents != static_cast<UINT>(input.size())) {
        logWindowWarning("Failed to simulate key events", wnd);
        return false;
    }
    return true;
}

} // namespace

WinPlatformWindow::WinPlatformWindow(HWND window)
    : m_window(window)
{
    GetWindowThreadProcessId(window, &m_processId);
}

bool WinPlatformWindow::matchesWindow(const QWindow *window) const
{
    return window && window->handle() && reinterpret_cast<HWND>(window->winId()) == m_window;
}

bool WinPlatformWindow::isValid() const
{
    DWORD processId = 0;
    return IsWindow(m_window) && IsWindowVisible(m_window) && GetWindowThreadProcessId(m_window, &processId)
        && processId == m_processId;
}

bool WinPlatformWindow::isActive() const
{
    return isValid() && GetForegroundWindow() == m_window;
}

bool WinPlatformWindow::pasteFromClipboardSafely(const std::function<bool()> &canPaste)
{
    const AppConfig config;
    return isActive() && (pasteWithCtrlV(*this, config)
        ? sendKeyPress(VK_LCONTROL, 'V', config, true, canPaste)
        : sendKeyPress(VK_LSHIFT, VK_INSERT, config, true, canPaste));
}

bool WinPlatformWindow::matchesWidget(const QWidget *widget) const
{
    return widget
        && widget->windowHandle()
        && reinterpret_cast<HWND>(widget->winId()) == m_window;
}


QString WinPlatformWindow::getTitle()
{
    return windowTitle(m_window);
}

QString WinPlatformWindow::getApplicationId() const
{
    if (!isValid()) return {};
    const auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_processId);
    if (!process) return {};
    WCHAR path[32768];
    DWORD length = DWORD(std::size(path));
    const bool success = QueryFullProcessImageNameW(process, 0, path, &length);
    CloseHandle(process);
    return success ? QString::fromWCharArray(path, int(length)) : QString();
}

void WinPlatformWindow::raise()
{
    raiseWindow(m_window);
}

bool WinPlatformWindow::pasteFromClipboard()
{
    const AppConfig config;

    if ( pasteWithCtrlV(*this, config) )
        return sendKeyPress(VK_LCONTROL, 'V', config);

    return sendKeyPress(VK_LSHIFT, VK_INSERT, config);
}

bool WinPlatformWindow::copyToClipboard()
{
    const AppConfig config;

    const DWORD clipboardSequenceNumber = GetClipboardSequenceNumber();
    if ( !sendKeyPress(VK_LCONTROL, 'C', config) )
        return false;

    // Wait for clipboard to change.
    QElapsedTimer t;
    t.start();
    while ( clipboardSequenceNumber == GetClipboardSequenceNumber() && t.elapsed() < 2000 )
        QApplication::processEvents(QEventLoop::AllEvents, 100);

    return clipboardSequenceNumber != GetClipboardSequenceNumber();
}

bool WinPlatformWindow::sendKeyPress(WORD modifier, WORD key, const AppConfig &config, bool requireFocus,
                                    const std::function<bool()> &canPaste)
{
    if (!requireFocus) {
        waitMs(config.option<Config::window_wait_before_raise_ms>());
        if (!raiseWindow(m_window))
            return false;
    }

    waitMs(config.option<Config::window_wait_after_raised_ms>());

    // Wait for user to release modifiers.
    if (!waitForModifiersReleased(config)) {
        logWindowWarning("Failed to simulate key presses while modifiers are pressed", m_window);
        return false;
    }

    if (requireFocus && (!isActive() || !canPaste()))
        return false;

    const int keyPressTimeMs = config.option<Config::window_key_press_time_ms>();
    if (keyPressTimeMs <= 0) {
        return sendInputs({
           createInput(modifier),
           createInput(key),
           createInput(key, KEYEVENTF_KEYUP),
           createInput(modifier, KEYEVENTF_KEYUP)
        }, m_window);
    }

    const bool sent = sendInputs({
        createInput(modifier),
        createInput(key)
    }, m_window);
    if (!sent)
        return false;

    waitMs(keyPressTimeMs);

    return sendInputs({
       createInput(key, KEYEVENTF_KEYUP),
       createInput(modifier, KEYEVENTF_KEYUP)
    }, m_window);
}
