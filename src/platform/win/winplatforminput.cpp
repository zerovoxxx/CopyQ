// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platforminput.h"
#include <qt_windows.h>
#include <imm.h>

namespace {
constexpr ULONG_PTR injectedTag = 0x51434c49;
class WinPlatformInput;
WinPlatformInput *listener = nullptr;
class WinPlatformInput final : public PlatformInput
{
public:
    ~WinPlatformInput() override { stop(); }
    bool start() override
    {
        if (m_hook) return true;
        if (listener) { m_error = tr("Input monitor already active."); return false; }
        listener = this;
        m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, callback, GetModuleHandleW(nullptr), 0);
        m_mouse = SetWindowsHookExW(WH_MOUSE_LL, mouseCallback, GetModuleHandleW(nullptr), 0);
        if (!m_hook || !m_mouse) { stop(); m_error = tr("Windows input monitoring is unavailable."); return false; }
        m_error.clear(); return true;
    }
    void stop() override
    {
        if (m_hook) UnhookWindowsHookEx(m_hook);
        if (m_mouse) UnhookWindowsHookEx(m_mouse);
        m_hook = m_mouse = nullptr;
        if (listener == this) listener = nullptr;
    }
    bool isSafe() const override
    {
        if (!m_hook) return false;
        const auto desktop = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
        if (!desktop) return false;
        wchar_t name[64]{};
        const bool normal = GetUserObjectInformationW(desktop, UOI_NAME, name, sizeof(name), nullptr) && QString::fromWCharArray(name) == QLatin1String("Default");
        CloseDesktop(desktop);
        const auto thread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        const auto layout = GetKeyboardLayout(thread);
        GUITHREADINFO gui{}; gui.cbSize = sizeof(gui);
        if (!GetGUIThreadInfo(thread, &gui) || !gui.hwndFocus) return false;
        wchar_t className[64]{};
        if (GetClassNameW(gui.hwndFocus, className, 64) && QString::fromWCharArray(className).compare(QLatin1String("Edit"),Qt::CaseInsensitive)==0
                && (GetWindowLongPtrW(gui.hwndFocus,GWL_STYLE) & ES_PASSWORD)) return false;
        // IMM does not provide reliable cross-process composition confirmation.
        return normal && layout && !ImmIsIME(layout);
    }
    bool sendKey(int key, int count, const std::function<bool()> &guard) override
    {
        const WORD code = key == Qt::Key_Backspace ? VK_BACK : key == Qt::Key_Left ? VK_LEFT : key == Qt::Key_Right ? VK_RIGHT : 0;
        if (!code || !isSafe()) return false;
        for (const auto modifier : {VK_CONTROL, VK_SHIFT, VK_MENU, VK_LWIN, VK_RWIN}) if (GetAsyncKeyState(modifier) & 0x8000) return false;
        for (int i = 0; i < count; ++i) {
            if (!guard() || !isSafe()) return false;
            INPUT events[2]{};
            events[0].type = events[1].type = INPUT_KEYBOARD;
            events[0].ki.wVk = events[1].ki.wVk = code;
            events[1].ki.dwFlags = KEYEVENTF_KEYUP;
            events[0].ki.dwExtraInfo = events[1].ki.dwExtraInfo = injectedTag;
            if (SendInput(2, events, sizeof(INPUT)) != 2) return false;
        }
        return true;
    }
private:
    static LRESULT CALLBACK mouseCallback(int code, WPARAM message, LPARAM event)
    {
        if (listener && code >= 0 && (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MOUSEWHEEL)) emit listener->reset();
        return CallNextHookEx(nullptr, code, message, event);
    }
    static LRESULT CALLBACK callback(int code, WPARAM message, LPARAM event)
    {
        if (listener && code >= 0) {
            auto self = listener;
            const auto native = reinterpret_cast<const KBDLLHOOKSTRUCT*>(event);
            if ((native->flags & LLKHF_LOWER_IL_INJECTED) || native->dwExtraInfo == injectedTag) return CallNextHookEx(nullptr, code, message, event);
            if (!self->isSafe()) { emit self->reset(); return CallNextHookEx(nullptr, code, message, event); }
            if ((message == WM_KEYUP || message == WM_SYSKEYUP) && (native->vkCode == VK_LCONTROL || native->vkCode == VK_RCONTROL || native->vkCode == VK_CONTROL)) emit self->input({}, Qt::Key_Control, {});
            if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
                Qt::KeyboardModifiers modifiers;
                BYTE state[256]{};
                for (int i = 0; i < 256; ++i) if (GetAsyncKeyState(i) & 0x8000) state[i] = 0x80;
                state[native->vkCode & 255] = 0x80;
                if (native->vkCode == VK_LCONTROL || native->vkCode == VK_RCONTROL) state[VK_CONTROL] = 0x80;
                if (native->vkCode == VK_LSHIFT || native->vkCode == VK_RSHIFT) state[VK_SHIFT] = 0x80;
                if (native->vkCode == VK_LMENU || native->vkCode == VK_RMENU) state[VK_MENU] = 0x80;
                state[VK_CAPITAL] = BYTE(GetKeyState(VK_CAPITAL) & 1);
                if (state[VK_CONTROL]) modifiers |= Qt::ControlModifier;
                if (state[VK_SHIFT]) modifiers |= Qt::ShiftModifier;
                if (state[VK_MENU]) modifiers |= Qt::AltModifier;
                if (state[VK_LWIN] || state[VK_RWIN]) modifiers |= Qt::MetaModifier;
                int key = native->vkCode == VK_BACK ? Qt::Key_Backspace : native->vkCode == 'C' ? Qt::Key_C : native->vkCode == 'V' ? Qt::Key_V : native->vkCode == VK_CONTROL || native->vkCode == VK_LCONTROL || native->vkCode == VK_RCONTROL ? Qt::Key_Control : 0;
                if (native->vkCode == VK_LEFT || native->vkCode == VK_RIGHT || native->vkCode == VK_UP || native->vkCode == VK_DOWN || native->vkCode == VK_HOME || native->vkCode == VK_END) { emit self->reset(); return CallNextHookEx(nullptr, code, message, event); }
                wchar_t text[16]{};
                const auto layout = GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), nullptr));
                const auto count = ToUnicodeEx(native->vkCode, native->scanCode, state, text, 16, 4, layout);
                if (count < 0) emit self->reset();
                else emit self->input(count > 0 ? QString::fromWCharArray(text, count) : QString(), key, modifiers);
            }
        }
        return CallNextHookEx(nullptr, code, message, event);
    }
    HHOOK m_hook = nullptr;
    HHOOK m_mouse = nullptr;
};
}
std::unique_ptr<PlatformInput> createPlatformInput() { return std::make_unique<WinPlatformInput>(); }
