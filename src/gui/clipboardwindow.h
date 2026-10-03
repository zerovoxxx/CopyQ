// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QQuickView>
#include <QtQml/qqmlregistration.h>

// Shared native material lifecycle; content remains in each existing controller.
class ClipboardWindow : public QQuickView
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Base class for QClip windows")
    Q_PROPERTY(bool blurAvailable READ blurAvailable NOTIFY materialChanged)
    Q_PROPERTY(QColor materialColor READ materialColor WRITE setMaterialColor NOTIFY materialChanged)
public:
    ClipboardWindow();
    bool blurAvailable() const { return m_blurAvailable; }
    QColor materialColor() const { return m_materialColor; }
    void setMaterialColor(const QColor &color);
signals:
    void materialChanged();
protected:
    bool event(QEvent *event) override;
    bool nativeEvent(const QByteArray &type, void *message, qintptr *result) override;
private:
    void updateMaterial();
    bool m_blurAvailable = false;
    QColor m_materialColor;
};
