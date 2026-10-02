// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <functional>
#include <memory>

// Native observation is deliberately limited to confirmed non-composing input.
class PlatformInput : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isSafe() const = 0;
    virtual bool sendKey(int key, int count, const std::function<bool()> &guard) = 0;
    QString error() const { return m_error; }
signals:
    void input(const QString &text, int key, Qt::KeyboardModifiers modifiers);
    void reset();
protected:
    QString m_error;
};
std::unique_ptr<PlatformInput> createPlatformInput();
