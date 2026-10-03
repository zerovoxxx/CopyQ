// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app.h"
#include "tests/clipboardguard.h"
#include "common/appconfig.h"
#include "common/common.h"
#include "common/contenttype.h"
#include "common/mimetypes.h"
#include "gui/clipboardpalette.h"
#include "gui/clipboarditempreview.h"
#include "gui/mainwindow.h"
#include "gui/theme.h"
#include "item/clipboardmodel.h"
#include "item/itemfactory.h"
#include "item/itemeditorwidget.h"
#include "platform/platformwindow.h"

#include <QApplication>
#include <QClipboard>
#include <QBuffer>
#include <QImage>
#include <QElapsedTimer>
#include <QInputMethodEvent>
#include <QMenu>
#include <QQuickItem>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QLineEdit>
#include <QProcess>
#include <QFile>
#include <QSaveFile>
#include <QDesktopServices>
#include <QDir>
#include <QScreen>
#include <QStandardItemModel>
#include <QQmlComponent>
#include <qscopeguard.h>

#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include "platform/mac/cfref.h"
#elif defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

namespace {
#if defined(Q_OS_WIN)
int observedInput = 0;
LRESULT CALLBACK inputHook(int code, WPARAM message, LPARAM event)
{
    if (code >= 0 && (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
            && (reinterpret_cast<KBDLLHOOKSTRUCT*>(event)->flags & LLKHF_INJECTED))
        ++observedInput;
    return CallNextHookEx(nullptr, code, message, event);
}
#elif defined(Q_OS_MACOS)
CGEventRef inputTap(CGEventTapProxy, CGEventType type, CGEventRef event, void *user)
{
    if (type == kCGEventKeyDown)
        ++*static_cast<int*>(user);
    return event;
}
#endif
QVariantMap textData(const QString &text)
{
    return {{mimeText, text.toUtf8()}};
}

class TestWindow final : public PlatformWindow
{
public:
    QString getTitle() override { return QStringLiteral("Controlled target"); }
    void raise() override { active = true; }
    bool pasteFromClipboard() override { return pasteFromClipboardSafely([]() { return true; }); }
    bool pasteFromClipboardSafely(const std::function<bool()> &canPaste) override
    { ++pastes; return valid && active && canPaste(); }
    bool copyToClipboard() override { return false; }
    bool matchesWidget(const QWidget *) const override { return false; }
    bool matchesWindow(const QWindow *) const override { return false; }
    bool isValid() const override { return valid; }
    bool isActive() const override { return valid && active; }
    bool valid = true;
    bool active = true;
    int pastes = 0;
};

class UrlHandler final : public QObject
{
    Q_OBJECT
public:
    QList<QUrl> opened;
public slots:
    void open(const QUrl &url) { opened.append(url); }
};
}

class PaletteTests final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        AppConfig config;
        config.setOption(QStringLiteral("filter_regular_expression"), false);
        config.setOption(QStringLiteral("filter_case_insensitive"), true);
        QVERIFY(m_factory.loadPlugins());
        const auto stats = m_factory.copyqStats().join('\n');
        QVERIFY2(stats.contains(QStringLiteral("PLUGIN itemtext: enabled")), qPrintable(stats));
        QVERIFY2(stats.contains(QStringLiteral("PLUGIN itemfakevim: enabled")), qPrintable(stats));
        QVERIFY2(stats.contains(QStringLiteral("PLUGIN itemimage: enabled")), qPrintable(stats));
        qInfo().noquote() << stats;
    }

    void glassWindowLifecycle()
    {
        ClipboardModel source;
        source.insertItem(textData(QStringLiteral("A quieter place for everything you copy.")), 0);
        source.insertItem(textData(QStringLiteral("https://github.com/p0deje/Maccy")), 1);
        source.insertItem(textData(QStringLiteral("把灵感留住，随时找回。")), 2);
        source.insertItem(textData(QStringLiteral("Meeting notes — Friday, 10:30")), 3);
        ClipboardPalette palette(&m_factory);
        Settings settings;
        auto theme = Theme(settings).quickTheme();
        theme.insert(QStringLiteral("custom_style"), false);
        palette.setTheme(theme);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History"), QStringLiteral("Work")}, {});
        QCOMPARE(palette.status(), QQuickView::Ready);
        QVERIFY(QTest::qWaitForWindowExposed(&palette));
        QVERIFY(palette.format().alphaBufferSize() >= 8);
        auto background = palette.rootObject()->property("background").value<QQuickItem*>();
        QVERIFY(background);
        QTRY_COMPARE(palette.materialColor(), theme.value(QStringLiteral("bg")).value<QColor>());
        const auto checkSurface = [&] {
            const auto color = background->property("color").value<QColor>();
            return palette.blurAvailable() ? color.alphaF() < 0.9 : color.alphaF() == 1.0;
        };
        QTRY_VERIFY(checkSurface());
        const auto screenshots = qEnvironmentVariable("COPYQ_TESTS_ARTIFACT_DIR");
        QWidget wallpaper;
        if (!screenshots.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshots));
            wallpaper.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
            wallpaper.setGeometry(palette.geometry().adjusted(-30, -30, 30, 30));
            wallpaper.setStyleSheet(QStringLiteral("background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #6a87d0,stop:0.4 #b7ccec,stop:0.65 #dcb8c3,stop:1 #bfdcc8);"));
            wallpaper.show();
            QVERIFY(QTest::qWaitForWindowExposed(&wallpaper));
            // Showing the controlled wallpaper may cancel the palette; open it again.
            palette.open(&source, QStringLiteral("History"), {QStringLiteral("History"), QStringLiteral("Work")}, {});
            palette.raise();
            QTest::qWait(250);
            qInfo() << "Native material available=" << palette.blurAvailable() << "active=" << palette.isActive();
            QVERIFY(palette.grabWindow().save(QDir(screenshots).filePath(QStringLiteral("palette-light.png"))));
            const auto rect = palette.geometry();
            const auto native = palette.screen()->grabWindow(0, rect.x(), rect.y(), rect.width(), rect.height());
            if (!native.isNull()) native.save(QDir(screenshots).filePath(QStringLiteral("palette-light-native.png")));
        }
        theme.insert(QStringLiteral("bg"), QColor(QStringLiteral("#25272d")));
        theme.insert(QStringLiteral("fg"), QColor(QStringLiteral("#f0f1f5")));
        palette.setTheme(theme);
        QTRY_COMPARE(palette.materialColor(), QColor(QStringLiteral("#25272d")));
        QTRY_VERIFY(checkSurface());
        auto preview = palette.rootObject()->findChild<ClipboardItemPreview *>(QStringLiteral("palette_preview"));
        QVERIFY(preview);
        QTRY_VERIFY(preview->previewWidget());
        QTRY_COMPARE(preview->previewWidget()->palette().color(QPalette::WindowText), QColor(QStringLiteral("#f0f1f5")));
        QTRY_COMPARE(preview->previewWidget()->parentWidget()->palette().color(QPalette::Base), QColor(QStringLiteral("#2e3036")));
        QTRY_COMPARE(preview->previewWidget()->parentWidget()->parentWidget()->palette().color(QPalette::Window), QColor(QStringLiteral("#2e3036")));
        for (auto child : preview->previewWidget()->findChildren<QWidget*>())
            QCOMPARE(child->palette().color(QPalette::Text), QColor(QStringLiteral("#f0f1f5")));
        if (!screenshots.isEmpty()) {
            QTest::qWait(250);
            QVERIFY(palette.grabWindow().save(QDir(screenshots).filePath(QStringLiteral("palette-dark.png"))));
            const auto rect = palette.geometry();
            const auto native = palette.screen()->grabWindow(0, rect.x(), rect.y(), rect.width(), rect.height());
            if (!native.isNull()) native.save(QDir(screenshots).filePath(QStringLiteral("palette-dark-native.png")));
        }
        palette.cancel();
        palette.destroy();
        QVERIFY(!palette.blurAvailable());
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QVERIFY(QTest::qWaitForWindowExposed(&palette));
        QTRY_VERIFY(checkSurface());
        QCOMPARE(palette.history()->count(), 4);
        palette.cancel();
    }

    void themedControls()
    {
        QQuickView view;
        QQmlComponent component(view.engine());
        component.setData(R"QML(
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QClip
Pane {
    id: root
    required property var values
    width: 560; height: 460; padding: 20
    Theme { id: theme; values: root.values }
    font: theme.textFont
    background: Rectangle { color: theme.background }
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        Label { text: "QClip · Controls"; color: theme.foreground; font.pixelSize: 18 }
        ThemeComboBox { id: combo; objectName: "control_combo"; values: root.values; Layout.fillWidth: true; model: ["History", "A long collection name · 中文收藏夹", "Work"] }
        ThemeTextField { values: root.values; Layout.fillWidth: true; iconName: "search"; placeholderText: "Search clipboard history…" }
        RowLayout {
            ThemeButton { values: root.values; text: "Copy"; iconName: "copy" }
            ThemeButton { values: root.values; text: "Paste"; primary: true; iconName: "return"; trailingIcon: true }
            ThemeButton { values: root.values; text: "Disabled"; enabled: false }
            ThemeButton { values: root.values; iconName: "more"; quiet: true; Accessible.name: "More" }
        }
        RowLayout {
            ThemeCheckBox { values: root.values; text: "Record history"; checked: true }
            ThemeCheckBox { values: root.values; text: "Disabled"; enabled: false }
            ThemeSpinBox { values: root.values; objectName: "control_spin"; value: 25; to: 100000; editable: true }
        }
        ThemeDelegate { values: root.values; Layout.fillWidth: true; text: "Selected collection"; highlighted: true }
        ThemeTextArea { values: root.values; Layout.fillWidth: true; Layout.fillHeight: true; text: "A quieter place for everything you copy.\n把灵感留住，随时找回。" }
    }
}
)QML", QUrl(QStringLiteral("qrc:/copyq/QClip/gui/qml/themed-controls.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        Settings settings;
        auto values = Theme(settings).quickTheme();
        auto root = qobject_cast<QQuickItem*>(component.createWithInitialProperties({{QStringLiteral("values"), values}}));
        QVERIFY2(root, qPrintable(component.errorString()));
        view.setContent(QUrl(), &component, root);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto combo = root->findChild<QQuickItem*>(QStringLiteral("control_combo"));
        QVERIFY(combo);
        auto indicator = combo->property("indicator").value<QQuickItem*>();
        QVERIFY(indicator);
        QVERIFY(qAbs(indicator->y() + indicator->height()/2 - combo->height()/2) < 0.1);
        auto popup = combo->property("popup").value<QObject*>();
        QVERIFY(popup);
        combo->forceActiveFocus();
        QTest::keyClick(&view, Qt::Key_Space);
        QTRY_VERIFY(popup->property("visible").toBool());
        auto list = popup->property("contentItem").value<QQuickItem*>();
        QVERIFY(list);
        QTRY_VERIFY(list->property("currentItem").value<QQuickItem*>());
        auto current = list->property("currentItem").value<QQuickItem*>();
        QVERIFY(current->width() <= popup->property("availableWidth").toReal());
        const auto artifacts = qEnvironmentVariable("COPYQ_TESTS_ARTIFACT_DIR");
        if (!artifacts.isEmpty()) {
            QTest::qWait(180);
            QVERIFY(view.grabWindow().save(artifacts + QStringLiteral("/controls-dropdown-light.png")));
        }
        QTest::keyClick(&view, Qt::Key_Down);
        QTest::keyClick(&view, Qt::Key_Return);
        QTRY_COMPARE(combo->property("currentIndex").toInt(), 1);
        QVERIFY(!popup->property("visible").toBool());
        auto spin = root->findChild<QQuickItem*>(QStringLiteral("control_spin"));
        QVERIFY(spin);
        spin->forceActiveFocus();
        QTest::keyClick(&view, Qt::Key_Up);
        QCOMPARE(spin->property("value").toInt(), 26);
        for (const bool dark : {false, true}) {
            if (dark) {
                values.insert(QStringLiteral("bg"), QColor(QStringLiteral("#25272d")));
                values.insert(QStringLiteral("fg"), QColor(QStringLiteral("#f0f1f5")));
                values.insert(QStringLiteral("edit_fg"), values.value(QStringLiteral("fg")));
                values.insert(QStringLiteral("edit_bg"), values.value(QStringLiteral("bg")));
                root->setProperty("values", values);
            }
            if (!artifacts.isEmpty()) {
                QTest::qWait(180);
                QVERIFY(view.grabWindow().save(artifacts + (dark ? QStringLiteral("/controls-dark.png") : QStringLiteral("/controls-light.png"))));
            }
        }
        auto font = values.value(QStringLiteral("font")).value<QFont>();
        font.setPixelSize(18);
        values.insert(QStringLiteral("font"), font);
        root->setProperty("values", values);
        QTRY_VERIFY(combo->height() >= 32);
        QVERIFY(qAbs(indicator->y() + indicator->height()/2 - combo->height()/2) < 0.1);
        if (!artifacts.isEmpty()) {
            QTest::qWait(180);
            QVERIFY(view.grabWindow().save(artifacts + QStringLiteral("/controls-large-font.png")));
        }
    }

    void collectionPopupNavigation()
    {
        ClipboardModel source;
        source.insertItem(textData(QStringLiteral("first")), 0);
        source.insertItem(textData(QStringLiteral("second")), 1);
        ClipboardPalette palette(&m_factory);
        Settings settings;
        palette.setTheme(Theme(settings).quickTheme());
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History"), QStringLiteral("Work")}, {});
        QVERIFY(QTest::qWaitForWindowExposed(&palette));
        auto combo = palette.rootObject()->findChild<QQuickItem*>(QStringLiteral("palette_sources"));
        QVERIFY(combo);
        combo->forceActiveFocus();
        QSignalSpy sourceChanged(&palette, &ClipboardPalette::sourceRequested);
        QSignalSpy activation(&palette, &ClipboardPalette::activationRequested);
        QTest::keyClick(&palette, Qt::Key_Space);
        QTRY_VERIFY(palette.rootObject()->property("popupActive").toBool());
        const auto artifacts = qEnvironmentVariable("COPYQ_TESTS_ARTIFACT_DIR");
        if (!artifacts.isEmpty()) {
            QTest::qWait(180);
            QVERIFY(palette.grabWindow().save(artifacts + QStringLiteral("/palette-dropdown.png")));
        }
        QTest::keyClick(&palette, Qt::Key_Down);
        QTest::keyClick(&palette, Qt::Key_Return);
        QTRY_COMPARE(sourceChanged.size(), 1);
        QCOMPARE(sourceChanged.first().first().toString(), QStringLiteral("Work"));
        QCOMPARE(palette.history()->selectedRow(), 0);
        QCOMPARE(activation.size(), 0);
        QVERIFY(palette.isVisible());
        QTest::keyClick(&palette, Qt::Key_Space);
        QTRY_VERIFY(palette.rootObject()->property("popupActive").toBool());
        QTest::keyClick(&palette, Qt::Key_Escape);
        QTRY_VERIFY(!palette.rootObject()->property("popupActive").toBool());
        QVERIFY(palette.isVisible());
        palette.cancel();
    }

    void modelIdentity()
    {
        ClipboardModel source;
        source.insertItem(textData(QStringLiteral("duplicate")), 0);
        source.insertItem(textData(QStringLiteral("duplicate")), 1);
        source.insertItem(textData(QStringLiteral("other")), 2);
        ClipboardPaletteModel model(&m_factory);
        model.setSourceModel(&source, QStringLiteral("History"));
        QTRY_VERIFY(!model.filtering());
        model.selectRow(1);
        const auto selected = model.selectedIndex();
        source.insertItem(textData(QStringLiteral("new")), 0);
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedIndex(), selected);
        QCOMPARE(model.selectedRow(), 2);
        QVERIFY(source.moveRows({}, selected.row(), 1, {}, 0));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedIndex(), selected);
        QCOMPARE(model.selectedRow(), 0);
        source.removeRow(selected.row());
        QTRY_VERIFY(!model.filtering());
        QVERIFY(!model.selectedIndex().isValid());
        QVERIFY(model.selectedData().isEmpty());
        // Deleting one duplicate must never activate the surviving duplicate.
        QCOMPARE(model.count(), 3);
        model.setSourceModel(nullptr, QString());
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 0);
    }

    void queryChanges()
    {
        ClipboardModel source;
        for (int i = 0; i < 3000; ++i)
            source.insertItem(textData(QStringLiteral("Alpha %1").arg(i)), i);
        source.insertItem(textData(QStringLiteral("中文检索")), 3000);
        source.insertItem(textData(QString(200000, QLatin1Char('x')) + QStringLiteral("needle")), 3001);
        ClipboardPaletteModel model(&m_factory);
        model.setSourceModel(&source, QStringLiteral("History"));
        model.setQuery(QStringLiteral("alpha"));
        QTest::qWait(1);
        model.setQuery(QStringLiteral("none"));
        model.setQuery(QStringLiteral("中文"));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.selectedData().value(mimeText).toByteArray(), QStringLiteral("中文检索").toUtf8());
        model.setQuery(QStringLiteral("needle"));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 1);
        QVERIFY(model.preview().value(QStringLiteral("text")).toString().size() <= 100000);
        model.setQuery(QStringLiteral("no match"));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 0);
        QCOMPARE(model.sourceCount(), 3002);
        model.setQuery(QString());
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 3002);
    }

    void displayCopiesAndPreview()
    {
        ClipboardModel source;
        const QVariantMap raw{{mimeText, QByteArray("original")}, {mimeHtml, QByteArray("<b>original</b>")},
                             {QStringLiteral("application/custom"), QByteArray("opaque")}};
        source.insertItem(raw, 0);
        ClipboardPaletteModel model(&m_factory);
        QSignalSpy display(&model, &ClipboardPaletteModel::itemDisplayRequested);
        model.setSourceModel(&source, QStringLiteral("History"));
        QTRY_VERIFY(!model.filtering());
        QVERIFY(!display.isEmpty());
        auto item = qvariant_cast<PersistentDisplayItem>(display.last().first());
        QVERIFY(item.isValid());
        item.setData(textData(QStringLiteral("presentation")));
        QCOMPARE(model.preview().value(QStringLiteral("text")).toString(), QStringLiteral("presentation"));
        QCOMPARE(model.selectedData(), raw);
        QCOMPARE(source.index(0).data(contentType::data).toMap(), raw);
        const auto imageReference = model.preview().value(QStringLiteral("image"));
        item.setData(textData(QStringLiteral("updated presentation")));
        QVERIFY(model.preview().value(QStringLiteral("image")) != imageReference);
        source.insertItem(textData(QStringLiteral("other")), 1);
        QTRY_VERIFY(!model.filtering());
        model.selectRow(1);
        QVERIFY(item.isValid());
        item.setData(textData(QStringLiteral("stale")));
        QCOMPARE(model.preview().value(QStringLiteral("text")).toString(), QStringLiteral("other"));
        QCOMPARE(model.data(model.index(0), ClipboardPaletteModel::SummaryRole).toString(), QStringLiteral("stale"));
        model.setDisplayEnabled(false);
        QVERIFY(!item.isValid());
        item.setData(textData(QStringLiteral("closed")));
        QCOMPARE(source.index(0).data(contentType::data).toMap(), raw);
        model.setDisplayEnabled(true);
        model.setSourceModel(nullptr, QString());
        QTRY_VERIFY(!model.filtering());
        QVERIFY(!item.isValid());
    }

    void sourceDestructionAndReset()
    {
        ClipboardPaletteModel model(&m_factory);
        auto source = std::make_unique<ClipboardModel>();
        source->insertItem(textData(QStringLiteral("one")), 0);
        model.setSourceModel(source.get(), QStringLiteral("First"));
        QTRY_VERIFY(!model.filtering());
        QVERIFY(model.selectedIndex().isValid());
        source.reset();
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 0);
        QVERIFY(!model.selectedIndex().isValid());
        ClipboardModel second;
        second.insertItem(textData(QStringLiteral("two")), 0);
        model.setSourceModel(&second, QStringLiteral("Second"));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedData().value(mimeText).toByteArray(), QByteArray("two"));
        QStandardItemModel resetSource(1, 1);
        resetSource.setData(resetSource.index(0, 0), textData(QStringLiteral("reset item")), contentType::data);
        ClipboardPalette palette(&m_factory);
        palette.open(&resetSource, QStringLiteral("Reset"), {QStringLiteral("Reset")}, {});
        QTRY_VERIFY(!palette.history()->filtering());
        palette.activate(false);
        QVERIFY(palette.busy());
        resetSource.clear();
        QTRY_VERIFY(!palette.busy());
        QVERIFY(!palette.pendingValid());
        QVERIFY(!palette.isVisible());
    }

    void qmlKeyboardAndIme()
    {
        ClipboardModel source;
        source.insertItem(textData(QStringLiteral("first")), 0);
        source.insertItem(textData(QStringLiteral("second")), 1);
        auto target = std::make_shared<TestWindow>();
        ClipboardPalette palette(&m_factory);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, target);
        QVERIFY2(palette.status() == QQuickView::Ready, qPrintable(palette.error()));
        QTRY_VERIFY(!palette.history()->filtering());
        auto search = palette.rootObject()->findChild<QQuickItem*>(QStringLiteral("palette_search"));
        QVERIFY(search);
        QTRY_VERIFY(search->hasActiveFocus());
        QSignalSpy activate(&palette, &ClipboardPalette::activationRequested);
        const auto originalClipboard = cloneData(QGuiApplication::clipboard()->mimeData());
        QTest::keyClick(&palette, Qt::Key_Down);
        QCOMPARE(palette.history()->selectedData().value(mimeText).toByteArray(), QByteArray("second"));
        QInputMethodEvent preedit(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(search, &preedit);
        QVERIFY(search->property("inputMethodComposing").toBool());
        QTest::keyClick(&palette, Qt::Key_Return);
        QCOMPARE(activate.count(), 0);
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("中文"));
        QCoreApplication::sendEvent(search, &commit);
        QTRY_VERIFY(!palette.history()->filtering());
        QCOMPARE(palette.history()->query(), QStringLiteral("中文"));
        QCOMPARE(palette.history()->count(), 0);
        QCOMPARE(cloneData(QGuiApplication::clipboard()->mimeData()), originalClipboard);
        QTest::keyClick(&palette, Qt::Key_Escape);
        QVERIFY(!palette.isVisible());
        QCOMPARE(activate.count(), 0);
    }

    void actionsAndCancellation()
    {
        ClipboardModel source;
        const QVariantMap raw{{mimeText, QByteArray("text")}, {mimeHtml, QByteArray("<b>text</b>")}};
        source.insertItem(raw, 0);
        auto target = std::make_shared<TestWindow>();
        ClipboardPalette palette(&m_factory);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, target);
        QTRY_VERIFY(!palette.history()->filtering());
        QSignalSpy activate(&palette, &ClipboardPalette::activationRequested);
        QSignalSpy cancelled(&palette, &ClipboardPalette::activationCancelled);
        palette.activate();
        palette.activate();
        QCOMPARE(activate.count(), 1);
        QCOMPARE(activate.last().at(1).toMap(), raw);
        QVERIFY(palette.busy());
        source.removeRow(0);
        QTRY_VERIFY(!palette.busy());
        QCOMPARE(cancelled.count(), 1);
        source.insertItem(raw, 0);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, target);
        QTRY_VERIFY(!palette.history()->filtering());
        palette.history()->selectRow(0);
        target->valid = false;
        palette.activate();
        QCOMPARE(activate.count(), 1);
        QVERIFY(!palette.error().isEmpty());
        palette.activate(false);
        QCOMPARE(activate.count(), 2);
        QCOMPARE(activate.last().at(2).toBool(), false);
        palette.complete(QStringLiteral("write failed"));
        QVERIFY(!palette.busy());
        QCOMPARE(palette.error(), QStringLiteral("write failed"));
        target->valid = true;
        palette.activate(true, true);
        QCOMPARE(activate.last().at(1).toMap(), textData(QStringLiteral("text")));
        palette.cancel();
    }

    void explicitCommands()
    {
        ClipboardModel source;
        source.insertItem(textData(QStringLiteral("text")), 0);
        ClipboardPalette palette(&m_factory);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, std::make_shared<TestWindow>());
        QTRY_VERIFY(!palette.history()->filtering());
        QMenu menu;
        auto command = menu.addAction(QStringLiteral("Custom Enter"));
        command->setShortcut(QKeySequence(Qt::Key_Return));
        menu.setDefaultAction(command);
        palette.setCommandMenu(&menu);
        QSignalSpy triggered(command, &QAction::triggered);
        QSignalSpy activate(&palette, &ClipboardPalette::activationRequested);
        QCOMPARE(palette.enterLabel(), QStringLiteral("Custom Enter"));
        palette.activate();
        QCOMPARE(triggered.count(), 1);
        QCOMPARE(activate.count(), 0);
        palette.activate(true, false, true);
        QCOMPARE(activate.count(), 1);
        palette.cancel();
    }

    void standardPreviews()
    {
        ClipboardModel source;
        QImage image(64, 64, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        QVERIFY(image.save(&buffer, "PNG"));
        source.insertItem({{QStringLiteral("image/png"), bytes}}, 0);
        ClipboardPalette palette(&m_factory);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QTRY_VERIFY(!palette.history()->filtering());
        QCOMPARE(palette.history()->preview().value(QStringLiteral("type")).toString(), QStringLiteral("Image"));
        QTest::qWait(100);
        QVERIFY(!palette.grabWindow().isNull());
        const auto screenshot = qEnvironmentVariable("COPYQ_TESTS_SCREENSHOT");
        if (!screenshot.isEmpty())
            QVERIFY(palette.grabWindow().save(screenshot));
        source.insertItem({{mimeUriList, QByteArray("file:///definitely-missing-qclip-test\r\nhttps://example.com\r\n")}}, 1);
        QTRY_VERIFY(!palette.history()->filtering());
        palette.history()->selectRow(1);
        QCOMPARE(palette.history()->preview().value(QStringLiteral("urls")).toStringList().size(), 2);
        palette.openUrl(QStringLiteral("file:///definitely-missing-qclip-test"));
        QVERIFY(!palette.error().isEmpty());
        const auto data = palette.history()->selectedData();
        palette.cancel();
        QCOMPARE(source.index(1).data(contentType::data).toMap(), data);
        source.insertItem({{mimeText, QByteArray("Link")},
            {mimeHtml, QByteArray("<a href=\"https://example.com/page\">Link</a>")}}, 2);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QTRY_VERIFY(!palette.history()->filtering());
        palette.history()->selectRow(2);
        QCOMPARE(palette.history()->preview().value(QStringLiteral("urls")).toStringList(),
                 QStringList{QStringLiteral("https://example.com/page")});
        UrlHandler handler;
        QDesktopServices::setUrlHandler(QStringLiteral("https"), &handler, "open");
        QDesktopServices::setUrlHandler(QStringLiteral("file"), &handler, "open");
        const auto cleanup = qScopeGuard([]() {
            QDesktopServices::unsetUrlHandler(QStringLiteral("https"));
            QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
        });
        QCOMPARE(handler.opened.size(), 0);
        palette.openUrl(QStringLiteral("https://example.com/unlisted"));
        QCOMPARE(handler.opened.size(), 0);
        palette.openUrl(QStringLiteral("https://example.com/page"));
        QCOMPARE(handler.opened, QList<QUrl>{QUrl(QStringLiteral("https://example.com/page"))});
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("preview.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("file preview");
        file.close();
        const auto fileUrl = QUrl::fromLocalFile(file.fileName());
        source.insertItem({{mimeUriList, fileUrl.toEncoded()}}, 3);
        QTRY_VERIFY(!palette.history()->filtering());
        palette.history()->selectRow(3);
        QCOMPARE(palette.history()->preview().value(QStringLiteral("filePath")).toString(), file.fileName());
        palette.openUrl(fileUrl.toString());
        QCOMPARE(handler.opened.last(), fileUrl);
        QCOMPARE(handler.opened.size(), 2);
        source.insertItem({{QStringLiteral("image/png"), QByteArray("invalid image")}}, 4);
        QTRY_VERIFY(!palette.history()->filtering());
        palette.history()->selectRow(4);
        auto preview = palette.rootObject()->findChild<ClipboardItemPreview*>(QStringLiteral("palette_preview"));
        QVERIFY(preview);
        QTRY_VERIFY(preview->previewWidget());
        QCOMPARE(palette.history()->selectedData().value(QStringLiteral("image/png")).toByteArray(), QByteArray("invalid image"));
    }

    void nativeWindowIdentification()
    {
        ClipboardModel source;
        ClipboardPalette palette(&m_factory);
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QTRY_VERIFY(palette.isExposed());
        auto native = platformNativeInterface()->getWindow(palette.winId());
        QVERIFY(native);
        QVERIFY(native->matchesWindow(&palette));
        QWidget other;
        other.show();
        QVERIFY(!native->matchesWidget(&other));
        palette.hide();
        // A different front window must be rejected before any native input.
        other.activateWindow();
        other.raise();
        QTest::qWait(100);
        QVERIFY(!native->isActive());
        QVERIFY(!native->pasteFromClipboardSafely([]() { return true; }));
        auto otherNative = platformNativeInterface()->getWindow(other.winId());
        otherNative->raise();
        QTRY_VERIFY(otherNative->isActive());
        QVERIFY(!otherNative->pasteFromClipboardSafely([]() { return false; }));
    }

    void pluginEditorAndSettings()
    {
        ItemEditorWidget plain({}, mimeText);
        plain.setPlainText(QStringLiteral("plain"));
        QVERIFY(!plain.data().contains(mimeHtml));
        ClipboardModel source;
        source.insertItem({{mimeText, QByteArray("rich")}, {mimeHtml, QByteArray("<b>rich</b>")}}, 0);
        ItemEditorWidget editor(source.index(0), mimeText);
        editor.setHtml(QStringLiteral("<b>rich</b>"));
        editor.moveCursor(QTextCursor::End);
        editor.insertPlainText(QStringLiteral(" changed"));
        QVERIFY(editor.data().contains(mimeHtml));
        QVERIFY(m_factory.setData(editor.data(), source.index(0), &source));
        QCOMPARE(source.index(0).data(contentType::text).toString(), QStringLiteral("rich changed"));
        bool settingsTested = false;
        for (const auto &loader : m_factory.loaders()) {
            if (loader->id() != QLatin1String("itemimage"))
                continue;
            QWidget parent;
            auto settingsWidget = loader->createSettingsWidget(&parent);
            QVERIFY(settingsWidget);
            QTemporaryDir dir;
            QSettings settings(dir.filePath(QStringLiteral("plugin.ini")), QSettings::IniFormat);
            loader->applySettings(settings);
            settings.sync();
            QCOMPARE(settings.status(), QSettings::NoError);
            QVERIFY(!settings.allKeys().isEmpty());
            settingsTested = true;
        }
        QVERIFY(settingsTested);
    }

    void performanceBaseline()
    {
        ClipboardModel source;
        for (int i = 0; i < 10000; ++i)
            source.insertItem(textData(QStringLiteral("history item %1").arg(i)), i);
        QElapsedTimer timer;
        timer.start();
        ClipboardPalette palette(&m_factory);
        QVERIFY(palette.load());
        const auto loadMs = timer.elapsed();
        palette.open(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QTRY_VERIFY(!palette.history()->filtering());
        const auto openMs = timer.elapsed();
        timer.restart();
        palette.history()->setQuery(QStringLiteral("9999"));
        QTRY_VERIFY(!palette.history()->filtering());
        const auto searchMs = timer.elapsed();
        QCOMPARE(palette.history()->count(), 1);
        qInfo() << "BASELINE items=10000 QML-load-ms=" << loadMs << "open-and-model-ms=" << openMs
                << "search-ms=" << searchMs << "RSS-bytes="
                << platformNativeInterface()->processResidentMemoryBytes(QCoreApplication::applicationPid());
    }

    void nativeInputListeningAndReplacement()
    {
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("input.txt"));
#ifdef Q_OS_MACOS
        if (!AXIsProcessTrusted() || IsSecureEventInputEnabled())
            QSKIP("Accessibility permission is unavailable or secure input is active; native input gate remains unverified.");
        int observedInput = 0;
        CFRef<CFMachPortRef> tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
            kCGEventTapOptionListenOnly, CGEventMaskBit(kCGEventKeyDown), inputTap, &observedInput);
        QVERIFY2(tap, "Input Monitoring permission or event tap creation failed");
        CFRef<CFRunLoopSourceRef> tapSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), tapSource, kCFRunLoopCommonModes);
        CGEventTapEnable(tap, true);
        const auto cleanupTap = qScopeGuard([&]() {
            CGEventTapEnable(tap, false);
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tapSource, kCFRunLoopCommonModes);
        });
#else
        observedInput = 0;
        const auto hook = SetWindowsHookExW(WH_KEYBOARD_LL, inputHook, GetModuleHandleW(nullptr), 0);
        QVERIFY(hook);
        const auto cleanupHook = qScopeGuard([&]() { UnhookWindowsHookEx(hook); });
#endif
        QProcess receiver;
        receiver.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--input-target"), path});
        QVERIFY(receiver.waitForStarted());
        const auto cleanupReceiver = qScopeGuard([&]() {
            if (receiver.state() != QProcess::NotRunning) {
                receiver.terminate();
                if (!receiver.waitForFinished(1000)) {
                    receiver.kill();
                    receiver.waitForFinished(1000);
                }
            }
        });
        QTRY_VERIFY(QFile::exists(path));
        PlatformWindowPtr target;
        const auto targetFocused = [&]() {
            target = platformNativeInterface()->getCurrentWindow();
            return target && target->getTitle().contains(QStringLiteral("QClip input target"));
        };
        QTRY_VERIFY(targetFocused());
        QVERIFY(target->isActive());
        // The listener stays in the background while native input reaches a separate controlled process.
        for (int i = 0; i < 5; ++i) {
            QVERIFY(target->isActive());
#ifdef Q_OS_MACOS
            CFRef<CGEventRef> down = CGEventCreateKeyboardEvent(nullptr, kVK_Delete, true);
            CFRef<CGEventRef> up = CGEventCreateKeyboardEvent(nullptr, kVK_Delete, false);
            CGEventSetFlags(down, 0);
            CGEventSetFlags(up, 0);
            CGEventPost(kCGHIDEventTap, down);
            CGEventPost(kCGHIDEventTap, up);
#else
            INPUT events[2]{};
            events[0].type = events[1].type = INPUT_KEYBOARD;
            events[0].ki.wVk = events[1].ki.wVk = VK_BACK;
            events[1].ki.dwFlags = KEYEVENTF_KEYUP;
            QCOMPARE(SendInput(2, events, sizeof(INPUT)), UINT(2));
#endif
            QTest::qWait(20);
        }
        for (const QChar character : QStringLiteral("expanded")) {
            QVERIFY(target->isActive());
#ifdef Q_OS_MACOS
            const UniChar value = character.unicode();
            CFRef<CGEventRef> down = CGEventCreateKeyboardEvent(nullptr, 0, true);
            CFRef<CGEventRef> up = CGEventCreateKeyboardEvent(nullptr, 0, false);
            CGEventKeyboardSetUnicodeString(down, 1, &value);
            CGEventKeyboardSetUnicodeString(up, 1, &value);
            CGEventSetFlags(down, 0);
            CGEventSetFlags(up, 0);
            CGEventPost(kCGHIDEventTap, down);
            CGEventPost(kCGHIDEventTap, up);
#else
            INPUT events[2]{};
            events[0].type = events[1].type = INPUT_KEYBOARD;
            events[0].ki.wScan = events[1].ki.wScan = character.unicode();
            events[0].ki.dwFlags = KEYEVENTF_UNICODE;
            events[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            QCOMPARE(SendInput(2, events, sizeof(INPUT)), UINT(2));
#endif
            QTest::qWait(20);
        }
        const auto read = [&path]() {
            QFile file(path);
            return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
        };
        QTRY_COMPARE(read(), QByteArray("expanded"));
        QVERIFY(observedInput >= 13);
        receiver.terminate();
        QVERIFY(receiver.waitForFinished());
#else
        QSKIP("Windows/macOS native input prototype; Linux protocol support belongs to Iteration3.");
#endif
    }

private:
    ItemFactory m_factory;
};

int main(int argc, char **argv)
{
    if (argc < 2 || qEnvironmentVariableIsEmpty("COPYQ_SETTINGS_PATH")
            || qEnvironmentVariableIsEmpty("COPYQ_ITEM_DATA_PATH") || qEnvironmentVariableIsEmpty("COPYQ_STATE_PATH")) {
        fprintf(stderr, "Use run-isolated.sh/ps1 and specify test functions.\n");
        return 2;
    }
    const bool inputTarget = argc == 3 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--input-target");
    std::unique_ptr<QApplication> app(inputTarget ? new QApplication(argc, argv)
        : platformNativeInterface()->createServerApplication(argc, argv));
    initSession(app.get(), QStringLiteral("test"));
    if (inputTarget) {
#ifdef Q_OS_WIN
        // The controlled receiver uses a confirmed non-IME layout. Do not
        // change the user's foreground application's input source.
        LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);
#endif
        const auto path = QString::fromLocal8Bit(argv[2]);
        QLineEdit receiver;
        QLineEdit second;
        second.setWindowTitle(QStringLiteral("QClip second target"));
        second.setText(QStringLiteral("other"));
        second.move(700, 250);
        receiver.setWindowTitle(QStringLiteral("QClip input target"));
        receiver.setText(QStringLiteral("qclip"));
        receiver.setCursorPosition(5);
        const auto write = [&]() {
            QSaveFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(receiver.text().toUtf8());
                file.commit();
            }
        };
        QObject::connect(&receiver, &QLineEdit::textChanged, &receiver, write);
        const auto writeSecond = [&]() {
            QSaveFile file(path + QStringLiteral(".other"));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(second.text().toUtf8());
                file.commit();
            }
        };
        QObject::connect(&second, &QLineEdit::textChanged, &second, writeSecond);
        second.show();
        writeSecond();
        receiver.show();
        receiver.activateWindow();
        receiver.raise();
        QTimer::singleShot(100, &receiver, [&]() {
            if (auto window = platformNativeInterface()->getWindow(receiver.winId()))
                window->raise();
#ifdef Q_OS_WIN
            LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);
#endif
            write();
        });
        QTimer control;
        QObject::connect(&control, &QTimer::timeout, &receiver, [&]() {
            QFile file(path + QStringLiteral(".control"));
            if (!file.open(QIODevice::ReadOnly))
                return;
            const auto command = file.readAll();
            file.close();
            file.remove();
            const auto acknowledge = [&]() {
                QSaveFile ack(path + QStringLiteral(".control-ack"));
                if (ack.open(QIODevice::WriteOnly)) {
                    ack.write(command);
                    ack.commit();
                }
            };
            if (command == "close-second") {
                second.close();
                acknowledge();
                return;
            }
            if (command == "select-first") { receiver.selectAll(); acknowledge(); return; }
            auto widget = command == "first" ? &receiver : &second;
            if (auto window = platformNativeInterface()->getWindow(widget->winId()))
                window->raise();
            acknowledge();
        });
        control.start(50);
        return app->exec();
    }
    const auto clipboard = backupClipboard();
    PaletteTests tests;
    const int result = QTest::qExec(&tests, argc, argv);
    restoreClipboard(clipboard);
    return result;
}

#include "tests_palette.moc"
