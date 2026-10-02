// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platforminput.h"
#include "x11platforminput.h"
#include <QElapsedTimer>
#include <QQueue>
#include <QHash>
#include <QGuiApplication>
#include <QSocketNotifier>
#ifdef COPYQ_WITH_XRECORD
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#endif

namespace {
struct InjectedKey { unsigned int code; bool pressed; qint64 expires; };
QQueue<InjectedKey> injectedKeys;
QElapsedTimer injectionClock;

class X11PlatformInput final : public PlatformInput
{
public:
    ~X11PlatformInput() override { stop(); }
    bool start() override
    {
#ifdef COPYQ_WITH_XRECORD
        if (m_context) return true;
        if (QGuiApplication::platformName() != QLatin1String("xcb")) { m_error = tr("Automatic expansion and copy gestures require X11; this Wayland compositor has no supported global input protocol."); return false; }
        m_control = XOpenDisplay(nullptr); m_events = XOpenDisplay(nullptr);
        int major = 0, minor = 0;
        if (!m_control || !m_events || !XRecordQueryVersion(m_control, &major, &minor)) { stop(); m_error = tr("X11 RECORD input monitoring is unavailable."); return false; }
        auto range = XRecordAllocRange();
        if (!range) { stop(); return false; }
        range->device_events.first = KeyPress;
        range->device_events.last = ButtonPress;
        XRecordClientSpec clients = XRecordAllClients;
        m_context = XRecordCreateContext(m_control, 0, &clients, 1, &range, 1);
        XFree(range);
        XSync(m_control, False);
        if (!m_context || !XRecordEnableContextAsync(m_events, m_context, callback, reinterpret_cast<XPointer>(this))) { stop(); m_error = tr("Unable to start X11 input monitor."); return false; }
        m_notifier = std::make_unique<QSocketNotifier>(ConnectionNumber(m_events), QSocketNotifier::Read);
        connect(m_notifier.get(), &QSocketNotifier::activated, this, [this] { XRecordProcessReplies(m_events); });
        m_error.clear(); return true;
#else
        m_error = tr("This build has no X11 RECORD/TEST input support."); return false;
#endif
    }
    void stop() override
    {
#ifdef COPYQ_WITH_XRECORD
        m_notifier.reset();
        if (m_control && m_context) { XRecordDisableContext(m_control, m_context); XRecordFreeContext(m_control, m_context); XSync(m_control, False); }
        m_context = 0; m_modifiers.clear();
        if (m_events) XCloseDisplay(m_events);
        if (m_control) XCloseDisplay(m_control);
        m_events = m_control = nullptr;
#endif
    }
    bool isSafe() const override
    {
#ifdef COPYQ_WITH_XRECORD
        const auto inputMethod = qEnvironmentVariable("XMODIFIERS");
        return m_context && (inputMethod.isEmpty() || inputMethod == QLatin1String("@im=none"))
            && qEnvironmentVariable("QT_IM_MODULE") != QLatin1String("ibus")
            && qEnvironmentVariable("QT_IM_MODULE") != QLatin1String("fcitx");
#else
        return false;
#endif
    }
    bool sendKey(int key, int count, const std::function<bool()> &guard) override
    {
#ifdef COPYQ_WITH_XRECORD
        const auto sym = key == Qt::Key_Backspace ? XK_BackSpace : key == Qt::Key_Left ? XK_Left : key == Qt::Key_Right ? XK_Right : NoSymbol;
        if (sym == NoSymbol || !isSafe()) return false;
        XkbStateRec state{};
        if (XkbGetState(m_control, XkbUseCoreKbd, &state) != Success || state.mods) return false;
        const auto code = XKeysymToKeycode(m_control, sym);
        if (!code) return false;
        for (int i = 0; i < count; ++i) {
            if (!guard() || !isSafe()) return false;
            recordInjectedX11Key(code, true); recordInjectedX11Key(code, false);
            if (!XTestFakeKeyEvent(m_control, code, True, CurrentTime) || !XTestFakeKeyEvent(m_control, code, False, CurrentTime)) return false;
        }
        XSync(m_control, False); return true;
#else
        Q_UNUSED(key); Q_UNUSED(count); Q_UNUSED(guard); return false;
#endif
    }
private:
#ifdef COPYQ_WITH_XRECORD
    static void callback(XPointer user, XRecordInterceptData *record)
    {
        auto self = reinterpret_cast<X11PlatformInput*>(user);
        if (record->category == XRecordFromServer && !record->client_swapped) {
            const auto count = record->data_len * 4 / sizeof(xEvent);
            const auto events = reinterpret_cast<const xEvent*>(record->data);
            for (unsigned long i = 0; i < count; ++i) {
                const auto &event = events[i];
                if (!self->isSafe()) { emit self->reset(); continue; }
                const auto type = event.u.u.type & 0x7f;
                while (!injectedKeys.isEmpty() && injectedKeys.head().expires < injectionClock.elapsed()) injectedKeys.dequeue();
                if (!injectedKeys.isEmpty() && injectedKeys.head().code == event.u.u.detail
                        && (type == KeyPress || type == KeyRelease) && injectedKeys.head().pressed == (type == KeyPress)) {
                    injectedKeys.dequeue(); continue;
                }
                XkbStateRec state{};
                if (XkbGetState(self->m_control, XkbUseCoreKbd, &state) != Success) { emit self->reset(); continue; }
                Qt::KeyboardModifiers modifiers;
                for (const auto modifier : self->m_modifiers) modifiers |= modifier;
                const auto sym = XkbKeycodeToKeysym(self->m_control, event.u.u.detail, state.group, modifiers & Qt::ShiftModifier ? 1 : 0);
                const auto modifier = sym == XK_Control_L || sym == XK_Control_R ? Qt::ControlModifier
                    : sym == XK_Shift_L || sym == XK_Shift_R ? Qt::ShiftModifier
                    : sym == XK_Alt_L || sym == XK_Alt_R ? Qt::AltModifier
                    : sym == XK_Super_L || sym == XK_Super_R || sym == XK_Meta_L || sym == XK_Meta_R ? Qt::MetaModifier : Qt::NoModifier;
                if (modifier != Qt::NoModifier) {
                    if (type == KeyPress) self->m_modifiers.insert(event.u.u.detail, modifier);
                    else if (type == KeyRelease) self->m_modifiers.remove(event.u.u.detail);
                    if (type == KeyPress && modifier == Qt::ControlModifier) emit self->input({}, Qt::Key_Control, Qt::ControlModifier);
                    if (type == KeyRelease && modifier == Qt::ControlModifier && !self->m_modifiers.values().contains(Qt::ControlModifier)) emit self->input({}, Qt::Key_Control, {});
                    continue;
                }
                if (type == KeyRelease) {
                    continue;
                }
                if (type != KeyPress) { emit self->reset(); continue; }
                const int key = sym == XK_BackSpace ? Qt::Key_Backspace : sym == XK_c || sym == XK_C ? Qt::Key_C : sym == XK_v || sym == XK_V ? Qt::Key_V : 0;
                if (sym == XK_Shift_L || sym == XK_Shift_R || sym == XK_Control_L || sym == XK_Control_R) continue;
                if (sym == XK_Left || sym == XK_Right || sym == XK_Up || sym == XK_Down || sym == XK_Home || sym == XK_End || (sym >= 0x100 && sym != XK_BackSpace)) { emit self->reset(); continue; }
                QString text;
                if (sym >= 0x20 && sym <= 0x7e) {
                    auto character = QChar(ushort(sym));
                    if ((state.locked_mods & LockMask) && character.isLetter()) character = character.isUpper() ? character.toLower() : character.toUpper();
                    text = character;
                }
                emit self->input(text, key, modifiers);
            }
        }
        XRecordFreeData(record);
    }
    Display *m_control = nullptr;
    Display *m_events = nullptr;
    XRecordContext m_context = 0;
    std::unique_ptr<QSocketNotifier> m_notifier;
    QHash<unsigned int, Qt::KeyboardModifier> m_modifiers;
#endif
};
}
void recordInjectedX11Key(unsigned int code, bool pressed)
{
    if (!injectionClock.isValid()) injectionClock.start();
    injectedKeys.enqueue({code, pressed, injectionClock.elapsed() + 1000});
    while (injectedKeys.size() > 2048) injectedKeys.dequeue();
}
std::unique_ptr<PlatformInput> createPlatformInput() { return std::make_unique<X11PlatformInput>(); }
