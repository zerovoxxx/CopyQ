// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platformwindoweffects.h"
#include <QSettings>
#include <QWindow>
#include <qt_windows.h>
#include <dwmapi.h>

bool setWindowBlur(QWindow *window, bool dark)
{
    const auto handle = reinterpret_cast<HWND>(window->winId());
    HIGHCONTRAST contrast{};
    contrast.cbSize = sizeof(contrast);
    SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    QSettings personalisation(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    BOOL composition = FALSE;
    DwmIsCompositionEnabled(&composition);
    const bool enabled = composition && !(contrast.dwFlags & HCF_HIGHCONTRASTON)
        && personalisation.value(QStringLiteral("EnableTransparency"), 1).toBool();
    // Numeric attributes keep builds compatible with older Windows SDK headers.
    // SYSTEMBACKDROP_TYPE (38) / TRANSIENTWINDOW (3): Windows 11 22621+ Acrylic.
    const int backdrop = enabled ? 3 : 1;
    const DWMNCRENDERINGPOLICY rendering = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(handle, DWMWA_NCRENDERING_POLICY, &rendering, sizeof(rendering));
    const BOOL useDark = dark;
    DwmSetWindowAttribute(handle, static_cast<DWMWINDOWATTRIBUTE>(20), &useDark, sizeof(useDark));
    const int corners = 2;
    DwmSetWindowAttribute(handle, static_cast<DWMWINDOWATTRIBUTE>(33), &corners, sizeof(corners));
    const bool available = SUCCEEDED(DwmSetWindowAttribute(handle,
        static_cast<DWMWINDOWATTRIBUTE>(38), &backdrop, sizeof(backdrop))) && enabled;
    // Windows 11 24H2+ can honour Qt Quick's premultiplied framebuffer alpha directly.
    const BOOL alpha = available;
    DwmSetWindowAttribute(handle, static_cast<DWMWINDOWATTRIBUTE>(39), &alpha, sizeof(alpha));
    const MARGINS margins = available ? MARGINS{-1, -1, -1, -1} : MARGINS{0, 0, 0, 0};
    if (FAILED(DwmExtendFrameIntoClientArea(handle, &margins))) return false;
    return available;
}
