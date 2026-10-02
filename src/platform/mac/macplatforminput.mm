// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platforminput.h"
#include "platform/mac/cfref.h"
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <climits>
#include <unistd.h>

namespace {
constexpr int64_t injectedTag = 0x51434c4950;
class MacPlatformInput final : public PlatformInput
{
public:
    ~MacPlatformInput() override { stop(); }
    bool start() override
    {
        if (m_tap) return true;
        if (!AXIsProcessTrusted()) { m_error = tr("Enable QClip in System Settings → Privacy & Security → Accessibility."); return false; }
        m_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
            CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged) | CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventRightMouseDown), callback, this);
        if (!m_tap) { m_error = tr("Enable QClip Input Monitoring permission, then enable expansion again."); return false; }
        m_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_tap, 0);
        if (!m_source) { stop(); m_error = tr("Unable to create input monitor."); return false; }
        CFRunLoopAddSource(CFRunLoopGetMain(), m_source, kCFRunLoopCommonModes);
        CGEventTapEnable(m_tap, true); m_error.clear(); return true;
    }
    void stop() override
    {
        if (m_tap) { CGEventTapEnable(m_tap, false); CFMachPortInvalidate(m_tap); }
        if (m_source) { CFRunLoopRemoveSource(CFRunLoopGetMain(), m_source, kCFRunLoopCommonModes); CFRelease(m_source); m_source = nullptr; }
        if (m_tap) { CFRelease(m_tap); m_tap = nullptr; }
    }
    bool isSafe() const override
    {
        if (!m_tap || !CGEventTapIsEnabled(m_tap) || !AXIsProcessTrusted() || IsSecureEventInputEnabled()) return false;
        CFRef<TISInputSourceRef> source = TISCopyCurrentKeyboardInputSource();
        if (!source) return false;
        const auto type = static_cast<CFStringRef>(TISGetInputSourceProperty(source, kTISPropertyInputSourceType));
        return type && CFEqual(type, kTISTypeKeyboardLayout);
    }
    bool sendKey(int key, int count, const std::function<bool()> &guard) override
    {
        const CGKeyCode code = key == Qt::Key_Backspace ? kVK_Delete : key == Qt::Key_Left ? kVK_LeftArrow : key == Qt::Key_Right ? kVK_RightArrow : USHRT_MAX;
        if (code == USHRT_MAX || !isSafe()) return false;
        if (CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState) & (kCGEventFlagMaskCommand | kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskShift)) return false;
        for (int i = 0; i < count; ++i) {
            if (!guard() || !isSafe()) return false;
            CFRef<CGEventRef> down = CGEventCreateKeyboardEvent(nullptr, code, true);
            CFRef<CGEventRef> up = CGEventCreateKeyboardEvent(nullptr, code, false);
            if (!down || !up) return false;
            CGEventSetFlags(down, 0); CGEventSetFlags(up, 0);
            CGEventSetIntegerValueField(down, kCGEventSourceUserData, injectedTag);
            CGEventSetIntegerValueField(up, kCGEventSourceUserData, injectedTag);
            CGEventPost(kCGHIDEventTap, down); CGEventPost(kCGHIDEventTap, up);
        }
        return true;
    }
private:
    static CGEventRef callback(CGEventTapProxy, CGEventType type, CGEventRef event, void *user)
    {
        auto self = static_cast<MacPlatformInput*>(user);
        if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
            emit self->reset(); // A disabled tap must not retain partially observed text.
            if (self->m_tap && AXIsProcessTrusted()) CGEventTapEnable(self->m_tap, true);
            return event;
        }
        if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == injectedTag
                || CGEventGetIntegerValueField(event, kCGEventSourceUnixProcessID) == getpid()) return event;
        if (!self->isSafe()) { emit self->reset(); return event; }
        const auto flags = CGEventGetFlags(event);
        Qt::KeyboardModifiers modifiers;
        if (flags & kCGEventFlagMaskCommand) modifiers |= Qt::ControlModifier;
        if (flags & kCGEventFlagMaskControl) modifiers |= Qt::MetaModifier;
        if (flags & kCGEventFlagMaskAlternate) modifiers |= Qt::AltModifier;
        if (flags & kCGEventFlagMaskShift) modifiers |= Qt::ShiftModifier;
        if (type == kCGEventFlagsChanged) {
            const auto code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            if (code == kVK_Command || code == kVK_RightCommand) emit self->input({}, Qt::Key_Control, modifiers);
            return event;
        }
        if (type != kCGEventKeyDown) { emit self->reset(); return event; }
        const auto code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        int key = code == kVK_Delete ? Qt::Key_Backspace : code == kVK_ANSI_C ? Qt::Key_C : code == kVK_ANSI_V ? Qt::Key_V : 0;
        UniChar buffer[16]; UniCharCount count = 0;
        CGEventKeyboardGetUnicodeString(event, 16, &count, buffer);
        const auto text = QString::fromUtf16(reinterpret_cast<const char16_t*>(buffer), qsizetype(count));
        if (code == kVK_LeftArrow || code == kVK_RightArrow || code == kVK_UpArrow || code == kVK_DownArrow || code == kVK_Home || code == kVK_End) { emit self->reset(); return event; }
        emit self->input(text, key, modifiers);
        return event;
    }
    CFMachPortRef m_tap = nullptr;
    CFRunLoopSourceRef m_source = nullptr;
};
}
std::unique_ptr<PlatformInput> createPlatformInput() { return std::make_unique<MacPlatformInput>(); }
