// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platformwindoweffects.h"
#include <QWindow>
#import <AppKit/AppKit.h>

bool setWindowBlur(QWindow *window, bool dark)
{
    NSView *content = reinterpret_cast<NSView *>(window->winId());
    NSWindow *native = content.window;
    if (!native) return false;
    NSVisualEffectView *effect = nil;
    for (NSView *view in content.subviews)
        if ([view.identifier isEqualToString:@"QClipWindowMaterial"])
            effect = (NSVisualEffectView *)view;
    if (NSWorkspace.sharedWorkspace.accessibilityDisplayShouldReduceTransparency) {
        [effect removeFromSuperview];
        return false;
    }
    if (!effect) {
        effect = [[NSVisualEffectView alloc] initWithFrame:content.bounds];
        effect.identifier = @"QClipWindowMaterial";
        effect.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [content addSubview:effect positioned:NSWindowBelow relativeTo:nil];
        [effect release];
    }
    effect.material = NSVisualEffectMaterialPopover;
    effect.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    effect.state = NSVisualEffectStateFollowsWindowActiveState;
    effect.appearance = [NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    native.opaque = NO;
    native.backgroundColor = NSColor.clearColor;
    native.hasShadow = YES;
    return true;
}
