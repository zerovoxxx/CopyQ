// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/platforminput.h"
namespace {
class DummyInput final : public PlatformInput
{
public:
    bool start() override { m_error = tr("Global input monitoring is unavailable on this platform."); return false; }
    void stop() override {}
    bool isSafe() const override { return false; }
    bool sendKey(int, int, const std::function<bool()> &) override { return false; }
};
}
std::unique_ptr<PlatformInput> createPlatformInput() { return std::make_unique<DummyInput>(); }
