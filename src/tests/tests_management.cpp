// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app.h"
#include "app/clipboardmonitor.h"
#include "common/appconfig.h"
#include "common/common.h"
#include "common/config.h"
#include "common/contenttype.h"
#include "common/mimetypes.h"
#include "common/historypolicy.h"
#include "common/encryption.h"
#include "common/settings.h"
#include "gui/clipboardbrowser.h"
#include "gui/clipboardbrowsershared.h"
#include "gui/clipboardmanagement.h"
#include "gui/clipboarditempreview.h"
#include "gui/clipboardstyle.h"
#include "gui/geometry.h"
#include "gui/clipboardpalette.h"
#include "gui/clipboardsettings.h"
#include "gui/clipboardcommands.h"
#include "gui/commandedit.h"
#include "gui/configurationmanager.h"
#include "item/clipboardmodel.h"
#include "item/itemfactory.h"
#include "item/serialize.h"
#include "platform/platformnativeinterface.h"
#include "tests/clipboardguard.h"

#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QInputMethodEvent>
#include <QMenu>
#include <QMimeData>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QPainter>
#include <QTextEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QListWidget>
#include <QTest>
#include <memory>

Q_DECLARE_METATYPE(QPersistentModelIndex)

namespace {
QVariantMap textData(const char *text)
{
    return {{mimeText, QByteArray(text)}};
}
}

class ManagementTests final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        ensureSettingsDirectoryExists();
        ensureStateDirectoryExists();
        m_shared = std::make_shared<ClipboardBrowserShared>();
        m_factory = std::make_unique<ItemFactory>(m_shared);
        m_shared->itemFactory = m_factory.get();
        m_shared->menuItems = menuItems();
        Settings settings;
        settings.beginGroup(QStringLiteral("Shortcuts"));
        loadShortcuts(&m_shared->menuItems, settings);
        settings.endGroup();
        m_shared->theme.loadTheme(settings);
        QVERIFY(m_factory->loadPlugins());
        const auto stats = m_factory->copyqStats().join('\n');
        for (const auto plugin : {"itemtext", "itemimage", "itemnotes", "itemtags", "itempinned", "itemsync", "itemfakevim", "itemencrypted"})
            QVERIFY2(stats.contains(QStringLiteral("PLUGIN %1: enabled").arg(QLatin1String(plugin))), qPrintable(stats));
        qInfo().noquote() << stats;
    }

    void multiSelectionIdentity()
    {
        ClipboardModel source;
        for (const auto value : {"duplicate", "duplicate", "other", "fourth"})
            source.insertItem(textData(value), source.rowCount());
        ClipboardPaletteModel model(m_factory.get());
        ClipboardPaletteModel palette(m_factory.get());
        model.setSourceModel(&source, QStringLiteral("History"));
        palette.setSourceModel(&source, QStringLiteral("History"));
        QTRY_VERIFY(!model.filtering() && !palette.filtering());
        model.select(0);
        model.select(2, true);
        QCOMPARE(model.selectedCount(), 3);
        model.select(1, false, true);
        QCOMPARE(model.selectedCount(), 2);
        auto selected = model.selectedIndexes();
        QCOMPARE(selected.at(0).row(), 0);
        QCOMPARE(selected.at(1).row(), 2);
        const auto paletteSelected = palette.selectedIndex();
        source.insertItem(textData("new"), 0);
        QTRY_VERIFY(!model.filtering() && !palette.filtering());
        QCOMPARE(model.selectedIndexes(), selected);
        QCOMPARE(palette.selectedIndex(), paletteSelected);
        QVERIFY(source.moveRows({}, selected.last().row(), 1, {}, 0));
        QTRY_VERIFY(!model.filtering());
        QVERIFY(model.selectedIndexes().contains(selected.last()));
        source.removeRow(selected.first().row());
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedCount(), 1);
        QCOMPARE(model.selectedIndexes().first(), selected.last());
        // The surviving duplicate must remain unselected after its sibling is deleted.
        model.setQuery(QStringLiteral("duplicate"));
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.count(), 1);
        model.selectAll();
        QCOMPARE(model.selectedCount(), 1);
        QCOMPARE(palette.query(), QString());
        model.setSelectedIndexes({});
        QCOMPARE(model.selectedCount(), 0);
        QVERIFY(!model.selectedIndex().isValid());
        source.insertItem(textData("unselected insertion"), 0);
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedCount(), 0);
        ClipboardModel empty;
        model.setQuery(QString());
        model.setSourceModel(&empty, QStringLiteral("InitiallyEmpty"));
        QTRY_VERIFY(!model.filtering());
        empty.insertItem(textData("first"), 0);
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedIndex(), QPersistentModelIndex(empty.index(0, 0)));
    }

    void bulkSelectionPerformance()
    {
        ClipboardModel source;
        source.insertItems(QVector<QVariantMap>(50000, textData("duplicate")), 0);
        ClipboardPaletteModel model(m_factory.get());
        model.setSourceModel(&source, QStringLiteral("Bulk"));
        QTRY_VERIFY(!model.filtering());
        QElapsedTimer timer;
        timer.start();
        model.selectAll();
        QCOMPARE(model.selectedCount(), 50000);
        const auto current = model.selectedIndex();
        source.insertItem(textData("new"), 0);
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedCount(), 50000);
        QCOMPARE(model.selectedIndex(), current);
        QVERIFY(!model.isSelected(0));
        qInfo() << "BULK items=50000 select-and-insert-ms=" << timer.elapsed();
        QVERIFY2(timer.elapsed() < 5000, "Bulk selection and identity restoration must remain responsive.");
    }

    void sourceLifetimeAndDisplay()
    {
        ClipboardPaletteModel model(m_factory.get());
        auto source = std::make_unique<ClipboardModel>();
        source->insertItem(textData("one"), 0);
        source->insertItem(textData("two"), 1);
        model.setSourceModel(source.get(), QStringLiteral("History"));
        QTRY_VERIFY(!model.filtering());
        model.selectRow(1);
        model.selectAll();
        auto item = PersistentDisplayItem(&model, model.selectedIndex(), model.displayRevision(), model.selectedData());
        item.setData(textData("displayed"));
        QCOMPARE(model.preview().value(QStringLiteral("text")).toString(), QStringLiteral("displayed"));
        QCOMPARE(source->index(1).data(contentType::data).toMap(), textData("two"));
        QSignalSpy display(&model, &ClipboardPaletteModel::itemDisplayRequested);
        source->setData(source->index(0), textData("one refreshed"), contentType::data);
        QTRY_VERIFY(!model.filtering());
        model.requestDisplay(0);
        QCOMPARE(display.size(), 1);
        auto unselected = qvariant_cast<PersistentDisplayItem>(display.first().first());
        QVERIFY(unselected.isValid());
        unselected.setData(textData("unselected presentation"));
        QCOMPARE(model.data(model.index(0), ClipboardPaletteModel::SummaryRole).toString(), QStringLiteral("unselected presentation"));
        QCOMPARE(model.preview().value(QStringLiteral("text")).toString(), QStringLiteral("displayed"));
        source->insertItem(textData("inserted"), 0);
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(display.size(), 1);
        QVERIFY(item.isValid());
        QVERIFY(unselected.isValid());
        model.requestDisplay(1);
        QCOMPARE(display.size(), 1);
        source->setData(source->index(1), textData("changed"), contentType::data);
        QTRY_VERIFY(!model.filtering());
        QVERIFY(!unselected.isValid());
        model.requestDisplay(1);
        QCOMPARE(display.size(), 2);
        model.setQuery(QStringLiteral("two"));
        QTRY_VERIFY(!model.filtering());
        QVERIFY(!item.isValid());
        source.reset();
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedCount(), 0);
        QVERIFY(!item.isValid());
        item.setData(textData("stale"));
        QVERIFY(model.preview().isEmpty());
        QStandardItemModel reset(2, 1);
        model.setSourceModel(&reset, QStringLiteral("Reset"));
        QTRY_VERIFY(!model.filtering());
        model.selectAll();
        reset.clear();
        QTRY_VERIFY(!model.filtering());
        QCOMPARE(model.selectedCount(), 0);
    }

    void transferAndDeleteProtection()
    {
        ClipboardBrowser source(QStringLiteral("TransferSource"), m_shared);
        ClipboardBrowser target(QStringLiteral("TransferTarget"), m_shared);
        QVERIFY(source.loadItems());
        QVERIFY(target.loadItems());
        auto raw = textData("rich");
        raw.insert(mimeHtml, QByteArray("<b>rich</b>"));
        raw.insert(mimeItemNotes, QByteArray("note"));
        raw.insert(QStringLiteral("application/custom"), QByteArray("opaque"));
        QVERIFY(source.add(raw));
        QVERIFY(source.add(textData("second")));
        QSignalSpy removed(&source, &ClipboardBrowser::runOnRemoveItemsHandler);
        QString error;
        QVERIFY(source.transferIndexes(&target, {source.index(1)}, 0, false, &error));
        QCOMPARE(source.length(), 2);
        QCOMPARE(target.index(0).data(contentType::data).toMap(), raw);
        QCOMPARE(removed.count(), 0);
        auto cancelled = connect(&source, &ClipboardBrowser::runOnRemoveItemsHandler,
            &source, [](const QList<QPersistentModelIndex> &, bool *canRemove) { *canRemove = false; });
        QVERIFY(!source.transferIndexes(&target, {source.index(0)}, 0, true, &error));
        QCOMPARE(target.length(), 1);
        QCOMPARE(source.length(), 2);
        QCOMPARE(removed.count(), 1);
        disconnect(cancelled);
        error.clear();
        QVERIFY(source.transferIndexes(&target, {source.index(0)}, 0, true, &error));
        QCOMPARE(source.length(), 1);
        QCOMPARE(target.length(), 2);
        QCOMPARE(removed.count(), 2);
        auto pinned = textData("protected");
        pinned.insert(QStringLiteral(COPYQ_MIME_PREFIX "item-pinned"), QByteArray());
        QVERIFY(source.add(pinned));
        const auto protectedIndex = QPersistentModelIndex(source.index(0));
        error.clear();
        QVERIFY(!source.transferIndexes(&target, {protectedIndex}, 0, true, &error));
        QVERIFY(error.contains(QStringLiteral("pinned")));
        QCOMPARE(target.length(), 2);
        source.removeIndexes({protectedIndex}, &error);
        QVERIFY(protectedIndex.isValid());
        const auto other = QPersistentModelIndex(source.index(1));
        source.removeIndexes({other}, &error);
        QVERIFY(!other.isValid());
        QVERIFY(protectedIndex.isValid());
        const auto foreign = target.index(0);
        QVERIFY(!source.transferIndexes(&target, {foreign}, 0, true, &error));
    }

    void qmlSelectionAndActions()
    {
        ClipboardModel source;
        source.insertItem(textData("first"), 0);
        source.insertItem(textData("second"), 1);
        ClipboardManagement window(m_factory.get());
        window.setActions(m_shared->menuItems);
        connect(&window, &ClipboardManagement::hideRequested, &window, &QWindow::hide);
        window.setTheme(m_shared->theme.quickTheme());
        window.setSource(&source, QStringLiteral("History"), {QStringLiteral("History")}, {});
        QVERIFY(window.load());
        window.open();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(!window.history()->filtering());
        const auto artifacts = qEnvironmentVariable("COPYQ_TESTS_ARTIFACT_DIR");
        if (!artifacts.isEmpty())
            QVERIFY(window.grabWindow().save(artifacts + QStringLiteral("/management.png")));
        window.resize(800, 520);
        QTRY_COMPARE(window.rootObject()->width(), qreal(800));
        QTest::qWait(50);
        for (const auto name : {"management_copy", "management_delete", "management_search", "management_results", "management_new_tab"}) {
            const auto control = window.rootObject()->findChild<QQuickItem*>(QLatin1String(name));
            QVERIFY(control);
            const auto bounds = control->mapRectToScene(QRectF(0, 0, control->width(), control->height()));
            QVERIFY2(QRectF(0, 0, 800, 520).contains(bounds), name);
        }
        if (!artifacts.isEmpty())
            QVERIFY(window.grabWindow().save(artifacts + QStringLiteral("/management-compact.png")));
        const auto newTab = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_new_tab"));
        QVERIFY(newTab);
        QSignalSpy tab(&window, &ClipboardManagement::tabRequested);
        QTest::mouseClick(&window, Qt::LeftButton, {}, newTab->mapToScene(QPointF(newTab->width()/2, newTab->height()/2)).toPoint());
        QTRY_VERIFY(window.rootObject()->property("popupActive").toBool());
        QTest::keyClick(&window, Qt::Key_Escape);
        QTRY_VERIFY(!window.rootObject()->property("popupActive").toBool());
        QVERIFY(window.isVisible());
        QCOMPARE(tab.count(), 0);
        window.triggerAction(Actions::Tabs_RenameTab);
        QTRY_VERIFY(window.rootObject()->property("popupActive").toBool());
        const auto tabName = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_tab_name"));
        QVERIFY(tabName);
        QTRY_VERIFY(tabName->hasActiveFocus());
        window.setSource(&source, QStringLiteral("Another"), {QStringLiteral("History"), QStringLiteral("Another")}, {});
        QInputMethodEvent rename;
        rename.setCommitString(QStringLiteral("Renamed"));
        QCoreApplication::sendEvent(window.focusObject(), &rename);
        QTest::keyClick(&window, Qt::Key_Return);
        QTRY_COMPARE(tab.count(), 1);
        QCOMPARE(tab.first().at(0).toString(), QStringLiteral("rename"));
        QCOMPARE(tab.first().at(1).toString(), QStringLiteral("History"));
        QCOMPARE(tab.first().at(2).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Renamed"));
        window.setSource(&source, QStringLiteral("Another"), {QStringLiteral("History"), QStringLiteral("Another")},
            {{QStringLiteral("maxItemCount"), 25}, {QStringLiteral("storeItems"), false}, {QStringLiteral("encryptedExpireSeconds"), 172800}});
        const auto properties = window.rootObject()->findChild<QObject*>(QStringLiteral("management_properties"));
        QVERIFY(properties);
        QVERIFY(QMetaObject::invokeMethod(properties, "open"));
        QTRY_VERIFY(window.rootObject()->property("popupActive").toBool());
        for (const auto key : {"management_capacity", "management_store", "management_encrypted_expiry"})
            QVERIFY(window.rootObject()->findChild<QObject*>(QLatin1String(key)));
        QCOMPARE(window.rootObject()->findChild<QObject*>(QStringLiteral("management_capacity"))->property("value").toInt(), 25);
        QCOMPARE(window.rootObject()->findChild<QObject*>(QStringLiteral("management_store"))->property("checked").toBool(), false);
        QCOMPARE(window.rootObject()->findChild<QObject*>(QStringLiteral("management_encrypted_expiry"))->property("value").toInt(), 172800);
        window.setSource(&source, QStringLiteral("History"), {QStringLiteral("History"), QStringLiteral("Another")}, {});
        QVERIFY(QMetaObject::invokeMethod(properties, "accept"));
        QTRY_COMPARE(tab.count(), 2);
        QCOMPARE(tab.last().at(0).toString(), QStringLiteral("properties"));
        QCOMPARE(tab.last().at(1).toString(), QStringLiteral("Another"));
        QCOMPARE(tab.last().at(2).toMap().value(QStringLiteral("encryptedExpireSeconds")).toInt(), 172800);
        QTRY_VERIFY(!window.history()->filtering());
        auto results = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_results"));
        QVERIFY(results);
        results->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Down, Qt::ShiftModifier);
        QCOMPARE(window.history()->selectedCount(), 2);
        QSignalSpy action(&window, &ClipboardManagement::actionRequested);
        const auto copy = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_copy"));
        QVERIFY(copy);
        QTest::mouseClick(&window, Qt::LeftButton, {}, copy->mapToScene(QPointF(copy->width()/2, copy->height()/2)).toPoint());
        QCOMPARE(action.count(), 1);
        QCOMPARE(action.first().at(0).toInt(), int(Actions::Edit_CopySelectedItems));
        QCOMPARE(action.first().at(2).value<QList<QPersistentModelIndex>>().size(), 2);
        QVERIFY(window.history()->selectedIndex().model() == &source);
        auto search = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_search"));
        QVERIFY(search);
        search->forceActiveFocus();
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("first"));
        QCoreApplication::sendEvent(window.focusObject(), &commit);
        QTRY_VERIFY(!window.history()->filtering());
        QCOMPARE(window.history()->count(), 1);
        QInputMethodEvent preedit(QStringLiteral("zhong"), {});
        QCoreApplication::sendEvent(window.focusObject(), &preedit);
        window.triggerAction(Actions::Item_Remove);
        QCOMPARE(action.count(), 1);
        QInputMethodEvent end;
        QCoreApplication::sendEvent(window.focusObject(), &end);
        QTest::keyClick(&window, Qt::Key_Escape);
        QTRY_COMPARE(window.history()->count(), 2);
        QCOMPARE(search->property("text").toString(), QString());
        results->forceActiveFocus();
        QTest::keyClick(&window, Qt::Key_Return);
        QCOMPARE(action.count(), 2);
        QCOMPARE(action.last().at(0).toInt(), int(Actions::Item_MoveToClipboard));
        QTest::keyClick(&window, Qt::Key_Escape);
        QVERIFY(!window.isVisible());
    }

    void nativeDropRoundTrip()
    {
        ClipboardBrowser target(QStringLiteral("DropTarget"), m_shared);
        QVERIFY(target.loadItems());
        ClipboardManagement window(m_factory.get());
        window.setTheme(m_shared->theme.quickTheme());
        window.setSource(target.model(), target.tabName(), {target.tabName()}, {});
        QVariantMap dropped;
        connect(&window, &ClipboardManagement::dataDropped, &target,
            [&target, &dropped](const QVariantMap &data, const QString &tab, int row, bool *accepted) {
                QCOMPARE(tab, target.tabName());
                dropped = data;
                *accepted = target.add(data, row);
            });
        window.open();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto results = window.rootObject()->findChild<QQuickItem*>(QStringLiteral("management_results"));
        QVERIFY(results);
        const auto position = results->mapToScene(QPointF(30, results->height() - 30)).toPoint();
        const QVariantMap raw{{mimeText, QByteArray("native drop")}, {mimeHtml, QByteArray("<b>native drop</b>")},
            {mimeItemNotes, QByteArray("note")}, {QStringLiteral("application/custom"), QByteArray("opaque")}};
        std::unique_ptr<QMimeData> mime(createMimeData(raw));
        QDragEnterEvent enter(position, Qt::CopyAction, mime.get(), Qt::LeftButton, {});
        QCoreApplication::sendEvent(&window, &enter);
        QVERIFY(enter.isAccepted());
        QDropEvent drop(position, Qt::CopyAction, mime.get(), Qt::LeftButton, {});
        QCoreApplication::sendEvent(&window, &drop);
        QVERIFY(drop.isAccepted());
        QCOMPARE(dropped, raw);
        QCOMPARE(target.length(), 1);
        QCOMPARE(target.index(0).data(contentType::data).toMap(), raw);
    }

    void themeMapping()
    {
        Settings settings;
        settings.beginGroup(QStringLiteral("ManagementTestTheme"));
        settings.setValue(QStringLiteral("bg"), QStringLiteral("#112233"));
        settings.setValue(QStringLiteral("fg"), QStringLiteral("#eef0ff"));
        settings.setValue(QStringLiteral("sel_bg"), QStringLiteral("#334455"));
        settings.setValue(QStringLiteral("font"), QStringLiteral("Arial,15"));
        settings.setValue(QStringLiteral("css"), QStringLiteral("QLabel { margin: 11px; }"));
        Theme theme(settings);
        const auto mapped = theme.quickTheme();
        QCOMPARE(mapped.value(QStringLiteral("bg")).value<QColor>(), QColor(QStringLiteral("#112233")));
        QCOMPARE(mapped.value(QStringLiteral("fg")).value<QColor>(), QColor(QStringLiteral("#eef0ff")));
        ClipboardManagement window(m_factory.get());
        window.setTheme(mapped);
        QVERIFY(window.load());
        QCOMPARE(window.rootObject()->property("font").value<QFont>(), theme.font(QStringLiteral("font")));
        const auto palette = window.rootObject()->property("palette").value<QObject*>();
        QVERIFY(palette);
        QCOMPARE(palette->property("text").value<QColor>(), QColor(QStringLiteral("#eef0ff")));
        Settings saved;
        saved.beginGroup(QStringLiteral("ManagementSavedTheme"));
        theme.saveTheme(&saved);
        QCOMPARE(saved.value(QStringLiteral("css")).toString(), QStringLiteral("QLabel { margin: 11px; }"));
    }

    void legacyStyleRules()
    {
        Settings settings;
        settings.beginGroup(QStringLiteral("LegacyStyleTest"));
        settings.setValue(QStringLiteral("style_main_window"), true);
        settings.setValue(QStringLiteral("fg"), QStringLiteral("#fedcba"));
        settings.setValue(QStringLiteral("css"), QStringLiteral("QPushButton { background: #123456; color: #fedcba; border: 0; } QPushButton:hover { background: #abcdef; }"));
        settings.setValue(QStringLiteral("sel_item_css"), QStringLiteral("background: #654321; border: 0;"));
        settings.setValue(QStringLiteral("quick_row_height"), 88);
        settings.setValue(QStringLiteral("menu_bar_selected_css"), QStringLiteral("background: #334455; border: 0;"));
        settings.setValue(QStringLiteral("tab_tree_sel_item_css"), QStringLiteral("background: #556677; border: 0;"));
        Theme theme(settings);
        ClipboardStyle control;
        control.setWidth(160);
        control.setHeight(48);
        control.setTheme(theme.quickTheme());
        const auto rendered = [&] {
            QImage image(160, 48, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            control.paint(&painter);
            return image;
        };
        QTRY_COMPARE(rendered().pixelColor(80, 24), QColor(QStringLiteral("#123456")));
        control.setHovered(true);
        QTRY_COMPARE(rendered().pixelColor(80, 24), QColor(QStringLiteral("#abcdef")));
        control.setHovered(false);
        control.setKind(ClipboardStyle::ToolbarButton);
        control.setText(QStringLiteral("Copy"));
        QTRY_COMPARE(control.foreground(), theme.color(QStringLiteral("fg")));
        const auto hasForeground = [&] {
            const auto image = rendered();
            const auto color = theme.color(QStringLiteral("fg"));
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const auto pixel = image.pixelColor(x, y);
                    if (pixel.alpha() > 128 && qAbs(pixel.red() - color.red()) < 20
                        && qAbs(pixel.green() - color.green()) < 20 && qAbs(pixel.blue() - color.blue()) < 20)
                        return true;
                }
            return false;
        };
        QTRY_VERIFY(hasForeground());
        QSignalSpy repainted(&control, &ClipboardStyle::metricsChanged);
        settings.setValue(QStringLiteral("style_main_window"), false);
        control.setTheme(Theme(settings).quickTheme());
        QTRY_VERIFY(!repainted.isEmpty());
        QTRY_VERIFY(hasForeground());
        control.setTheme(theme.quickTheme());
        control.setText({});
        control.setKind(ClipboardStyle::Item);
        control.setSelected(true);
        QTRY_COMPARE(rendered().pixelColor(80, 24), QColor(QStringLiteral("#654321")));
        control.setKind(ClipboardStyle::Collection);
        QTRY_COMPARE(rendered().pixelColor(80, 24), QColor(QStringLiteral("#556677")));
        control.setSelected(false);
        control.setKind(ClipboardStyle::MenuItem);
        control.setHovered(true);
        QTRY_COMPARE(rendered().pixelColor(80, 24), QColor(QStringLiteral("#334455")));
        QCOMPARE(theme.quickTheme().value(QStringLiteral("quick_row_height")).toInt(), 88);
        Settings saved;
        saved.beginGroup(QStringLiteral("LegacyStyleSaved"));
        theme.saveTheme(&saved);
        Theme loaded(saved);
        QCOMPARE(loaded.quickTheme().value(QStringLiteral("items_css")), theme.quickTheme().value(QStringLiteral("items_css")));
        QCOMPARE(loaded.value(QStringLiteral("css")), theme.value(QStringLiteral("css")));
    }

    void settingsDraftTransaction()
    {
        AppConfig initial;
        initial.setOption(Config::maxitems::name(), 321);
        initial.setOption(Config::check_clipboard::name(), false);
        ConfigurationManager original;
        const auto originalNames = original.options();
        {
            ClipboardSettings window(m_shared);
            QSet<QString> names;
            for (const auto &field : window.fields())
                names.insert(field.toMap().value(QStringLiteral("name")).toString());
            QCOMPARE(names, QSet<QString>(originalNames.begin(), originalNames.end()));
            QCOMPARE(window.plugins().size(), 8);
            window.open();
            QCOMPARE(window.status(), QQuickView::Ready);
            QVERIFY(window.rootObject());
            QQuickItem *control = nullptr;
            const auto findControl = [&]() {
                QList<QQuickItem*> items{window.rootObject()};
                while (!items.isEmpty()) {
                    auto item = items.takeLast();
                    if (item->objectName() == QLatin1String("settings_maxitems")) {
                        control = item;
                        return true;
                    }
                    items.append(item->childItems());
                }
                return false;
            };
            QTRY_VERIFY(findControl());
            QCOMPARE(control->property("text").toString(), QStringLiteral("321"));
            QVERIFY(window.setValue(QStringLiteral("maxitems"), 900));
            QVERIFY(window.setValue(QStringLiteral("check_clipboard"), true));
            AppConfig unchanged;
            QCOMPARE(unchanged.option<Config::maxitems>(), 321);
            QCOMPARE(unchanged.option<Config::check_clipboard>(), false);
            window.cancel();
        }
        {
            ClipboardSettings window(m_shared);
            QVERIFY(!window.setValue(QStringLiteral("maxitems"), QStringLiteral("invalid")));
            QVERIFY(!window.error().isEmpty());
            QVERIFY(window.setValue(QStringLiteral("maxitems"), 900));
            QSignalSpy changed(&window, &ClipboardSettings::configurationChanged);
            window.apply();
            QCOMPARE(changed.count(), 1);
            AppConfig saved;
            QCOMPARE(saved.option<Config::maxitems>(), 900);
            window.resetDefaults();
            AppConfig beforeDefaultsApply;
            QCOMPARE(beforeDefaultsApply.option<Config::maxitems>(), 900);
            window.apply();
            AppConfig defaults;
            QCOMPARE(defaults.option<Config::maxitems>(), Config::maxitems::defaultValue());
        }
    }

    void pluginPreviewBridge()
    {
        ClipboardModel source;
        const QVariantMap rich{{mimeText, QByteArray("plain")}, {mimeHtml, QByteArray("<b>rich preview</b>")},
            {mimeItemNotes, QByteArray("preview note")}, {QStringLiteral("application/custom"), QByteArray("opaque")}};
        source.insertItem(rich, 0);
        ClipboardPaletteModel model(m_factory.get());
        model.setSourceModel(&source, QStringLiteral("Preview"));
        QTRY_VERIFY(!model.filtering());
        ClipboardItemPreview preview;
        preview.setWidth(320);
        preview.setHeight(200);
        preview.setHistory(&model);
        const auto rendered = [&]() {
            QImage image(320, 200, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            preview.paint(&painter);
            return image;
        };
        QTRY_VERIFY(rendered().pixelColor(10, 10).alpha() > 0);
        auto texts = preview.previewWidget()->findChildren<QTextEdit *>();
        if (auto text = qobject_cast<QTextEdit *>(preview.previewWidget())) texts.append(text);
        bool richTextShown = false;
        for (const auto text : texts) {
            QVERIFY(text->isReadOnly());
            richTextShown = richTextShown || text->toPlainText().contains(QStringLiteral("rich preview"));
        }
        QVERIFY(richTextShown);
        const auto first = rendered();
        QPointer<QWidget> old = preview.previewWidget();
        model.setDisplayData(model.selectedIndex(), model.displayRevision(), textData("display script result"));
        QVERIFY(!old);
        QTRY_VERIFY(rendered() != first);
        QCOMPARE(source.index(0).data(contentType::data).toMap(), rich);
        QImage image(64, 48, QImage::Format_RGB32);
        image.fill(Qt::red);
        QByteArray png;
        QBuffer buffer(&png);
        QVERIFY(buffer.open(QIODevice::WriteOnly));
        QVERIFY(image.save(&buffer, "PNG"));
        source.insertItem({{QStringLiteral("image/png"), png}}, 0);
        QTRY_VERIFY(!model.filtering());
        model.selectRow(0);
        QTRY_VERIFY(rendered() != first);
        QVERIFY(preview.previewWidget());
        QPointer<QWidget> last = preview.previewWidget();
        model.setSourceModel(nullptr, QString());
        QVERIFY(!last);
        QVERIFY(!preview.previewWidget());
    }

    void managementGeometry()
    {
        AppConfig config;
        config.setOption(Config::restore_geometry::name(), true);
        config.setOption(Config::open_windows_on_current_screen::name(), false);
        const auto key = QStringLiteral("Options/clipboard_management_geometry_global");
        const QRect saved(100, 160, 900, 600);
        setGeometryOptionValue(key, saved);
        setGeometryOptionValue(key + QStringLiteral("_maximized"), false);
        ClipboardManagement first(m_factory.get());
        first.open();
        QTRY_COMPARE(first.geometry(), saved);
        const QRect moved(120, 180, 850, 560);
        first.setGeometry(moved);
        first.hide();
        QCOMPARE(geometryOptionValue(key).toRect(), moved);
        first.openAt(QRect(200, 200, 600, 400));
        QTRY_COMPARE(first.position(), QPoint(200, 200));
        first.hide();
        QCOMPARE(geometryOptionValue(key).toRect(), moved);
        ClipboardManagement second(m_factory.get());
        second.open();
        QTRY_COMPARE(second.geometry(), moved);
        second.hide();
        config.setOption(Config::restore_geometry::name(), false);
    }

    void commandDraftRoundTrip()
    {
        Command original;
        original.name = QStringLiteral("First");
        original.cmd = QStringLiteral("copyq: data('text/plain')");
        // The existing INI format stores regex patterns; use its inline options.
        original.re = QRegularExpression(QStringLiteral("(?i)test"));
        original.wndre = QRegularExpression(QStringLiteral("(?s)window"));
        original.internalId = QStringLiteral("preserved-id");
        original.nameLocalization.insert(QStringLiteral("zh_CN"), QStringLiteral("测试"));
        original.shortcuts = {QStringLiteral("Return"), QStringLiteral("Ctrl+1")};
        original.globalShortcuts = {QStringLiteral("Ctrl+Alt+1")};
        original.tab = QStringLiteral("Group/Target");
        original.input = mimeText;
        original.output = mimeHtml;
        original.sep = QStringLiteral("\\n");
        original.inMenu = original.transform = original.wait = original.hideWindow = true;
        saveCommands({original});
        {
            ClipboardCommands window(m_shared);
            window.open();
            QCOMPARE(window.status(), QQuickView::Ready);
            QVERIFY(window.rootObject());
            QCOMPARE(window.fields().size(), 23);
            QVERIFY(window.setField(QStringLiteral("name"), QStringLiteral("Discarded")));
            QVERIFY(window.modified());
            window.cancel();
        }
        QCOMPARE(loadAllCommands(), Commands{original});
        {
            ClipboardCommands window(m_shared);
            QVERIFY(window.setField(QStringLiteral("name"), QStringLiteral("Saved")));
            QVERIFY(window.setField(QStringLiteral("shortcuts"), QStringLiteral("Return\nCtrl+2")));
            original.name = QStringLiteral("Saved");
            original.shortcuts = {QStringLiteral("Return"), QStringLiteral("Ctrl+2")};
            QVERIFY(window.apply());
            QCOMPARE(loadAllCommands(), Commands{original});
            const auto exported = window.exportSelected({0});
            QCOMPARE(importCommandsFromText(exported), Commands{original});
            QVERIFY(!window.importText(QStringLiteral("invalid")));
            QVERIFY(window.importText(exported));
            QCOMPARE(window.commands().size(), 2);
            window.move(1, -1);
            QCOMPARE(window.currentIndex(), 0);
            window.remove({0, 0, -1, 99});
            QCOMPARE(window.commands().size(), 1);
            QVERIFY(window.setField(QStringLiteral("cmd"), QStringLiteral("copyq: if (")));
            QVERIFY(!window.apply());
            QVERIFY(!window.error().isEmpty());
            QCOMPARE(loadAllCommands(), Commands{original});
            QVERIFY(window.setField(QStringLiteral("enable"), false));
            QVERIFY(window.apply());
            QVERIFY(window.setField(QStringLiteral("enable"), true));
            QVERIFY(window.setField(QStringLiteral("cmd"), QStringLiteral("copyq: throw new Error('never run while checking')")));
            QVERIFY(window.apply());
            QVERIFY(!window.modified());
        }
        QVERIFY(CommandEdit::scriptError(QStringLiteral("copyq: }\n globalThis.executed = true; { ")).size() > 0);
        QVERIFY(CommandEdit::scriptError(QStringLiteral("copyq: return; throw new Error('never run')")).isEmpty());
        QVERIFY(CommandEdit::scriptError(QStringLiteral("sh: echo '('")).isEmpty());
    }

    void pluginSettingsDraft()
    {
        Settings initial;
        initial.beginGroup(QStringLiteral("Plugins"));
        initial.beginGroup(QStringLiteral("itemtext"));
        initial.setValue(QStringLiteral("use_rich_text"), true);
        const auto checkBox = []() -> QCheckBox* {
            for (auto widget : QApplication::topLevelWidgets()) {
                if (widget->objectName() == QLatin1String("qclip_settings_native"))
                    return widget->findChild<QCheckBox*>(QStringLiteral("checkBoxUseRichText"));
            }
            return nullptr;
        };
        {
            ClipboardSettings window(m_shared);
            window.openPage(QStringLiteral("itemtext"));
            auto box = checkBox();
            QVERIFY(box);
            QVERIFY(box->isChecked());
            box->setChecked(false);
            window.cancel();
        }
        Settings afterCancel;
        afterCancel.beginGroup(QStringLiteral("Plugins"));
        afterCancel.beginGroup(QStringLiteral("itemtext"));
        QCOMPARE(afterCancel.value(QStringLiteral("use_rich_text")).toBool(), true);
        {
            ClipboardSettings window(m_shared);
            window.openPage(QStringLiteral("itemtext"));
            auto box = checkBox();
            QVERIFY(box);
            QVERIFY(box->isChecked());
            box->setChecked(false);
            connect(&window, &ClipboardSettings::configurationChanged, this, [this] {
                Settings settings;
                m_factory->loadItemFactorySettings(&settings);
            });
            window.apply();
        }
        Settings afterApply;
        afterApply.beginGroup(QStringLiteral("Plugins"));
        afterApply.beginGroup(QStringLiteral("itemtext"));
        QCOMPARE(afterApply.value(QStringLiteral("use_rich_text")).toBool(), false);
        {
            ClipboardSettings window(m_shared);
            window.openPage(QStringLiteral("itemtext"));
            auto box = checkBox();
            QVERIFY(box);
            QVERIFY(!box->isChecked());
            window.cancel();
        }
        afterApply.setValue(QStringLiteral("use_rich_text"), true);
        Settings restored;
        m_factory->loadItemFactorySettings(&restored);
    }

    void allPluginSettings()
    {
        ConfigurationManager configuration(m_shared);
        for (const auto &field : configuration.pluginFields()) {
            const auto id = field.toMap().value(QStringLiteral("id")).toString();
            auto page = configuration.settingsPage(id);
            QVERIFY2(page, qPrintable(id));
            if (id != QLatin1String("itempinned"))
                QVERIFY2(page->findChild<QScrollArea *>(QStringLiteral("plugin_settings_form")), qPrintable(id));
        }
        auto appearance = configuration.settingsPage(QStringLiteral("Appearance"));
        QVERIFY(appearance->findChild<QSpinBox *>(QStringLiteral("quick_row_height")));
        auto shortcuts = configuration.settingsPage(QStringLiteral("Shortcuts"));
        QVERIFY(shortcuts->findChild<QListWidget *>(QStringLiteral("shortcut_navigation")));
        QVERIFY(configuration.settingsPage(QStringLiteral("Tabs")));
    }

    void historyTimeAndProtection()
    {
        const qint64 now = 1800000000000;
        const qint64 range = 60000;
        const auto item = [](const char *text, qint64 time) {
            auto data = textData(text);
            if (time >= 0) data.insert(mimeHistoryTime, QByteArray::number(time));
            return data;
        };
        QVERIFY(HistoryPolicy::isRecent(item("now", now), now, range));
        QVERIFY(HistoryPolicy::isRecent(item("edge", now - range), now, range));
        QVERIFY(!HistoryPolicy::isRecent(item("older", now - range - 1), now, range));
        QVERIFY(!HistoryPolicy::isRecent(item("future", now + 1), now, range));
        QVERIFY(!HistoryPolicy::isRecent(item("legacy", -1), now, range));
        QVERIFY(!HistoryPolicy::isExpired(item("future", now + 1), now, range));
        QVERIFY(!HistoryPolicy::isExpired(item("edge", now - range), now, range));
        QVERIFY(HistoryPolicy::isExpired(item("old", now - range - 1), now, range));
        QVERIFY(!HistoryPolicy::isExpired(item("legacy", -1), now, range));
        QVERIFY(!HistoryPolicy::isExpired(item("old", now - range - 1), now - range - 2, range));
        ClipboardBrowser source(QStringLiteral("TimedHistory"), m_shared);
        QVERIFY(source.loadItems());
        auto raw = item("original", now - range - 1);
        raw.insert(mimeHtml, QByteArray("<b>original</b>"));
        raw.insert(QStringLiteral("application/custom"), QByteArray("opaque"));
        raw.insert(mimeSourceApplication, QByteArray("source.one"));
        source.addUnique(raw, ClipboardMode::Clipboard);
        source.addUnique(textData("other"), ClipboardMode::Clipboard);
        auto repeated = raw;
        repeated.insert(mimeHistoryTime, QByteArray::number(now));
        repeated.insert(mimeSourceApplication, QByteArray("source.two"));
        source.addUnique(repeated, ClipboardMode::Clipboard);
        QCOMPARE(source.length(), 2);
        QCOMPARE(source.index(0).data(contentType::data).toMap(), repeated);
        QVERIFY(!source.refreshHistoryMetadata(item("absent", now)));
        QCOMPARE(source.length(), 2);
        auto pinned = item("pinned", now);
        pinned.insert(QStringLiteral(COPYQ_MIME_PREFIX "item-pinned"), QByteArray());
        QVERIFY(source.add(pinned));
        auto recent = HistoryPolicy::candidates(source.model(), now, range, HistoryPolicy::Cleanup::Recent);
        QCOMPARE(recent.size(), 1);
        QCOMPARE(recent.first().data(contentType::text).toString(), QStringLiteral("original"));
        const auto prevent = connect(&source, &ClipboardBrowser::runOnRemoveItemsHandler,
            &source, [](const QList<QPersistentModelIndex> &, bool *allowed) { *allowed = false; });
        source.removeIndexes({recent.first()});
        QCOMPARE(source.length(), 3);
        disconnect(prevent);
        source.removeIndexes({recent.first()});
        QCOMPARE(source.length(), 2);
        auto all = HistoryPolicy::candidates(source.model(), now, 0, HistoryPolicy::Cleanup::All);
        QCOMPARE(all.size(), 1);
        QCOMPARE(all.first().data(contentType::text).toString(), QStringLiteral("other"));
        QBuffer serialized;
        QVERIFY(serialized.open(QIODevice::ReadWrite));
        QVERIFY(serializeData(*source.model(), &serialized));
        serialized.seek(0);
        ClipboardModel restored;
        QVERIFY(deserializeData(&restored, &serialized));
        QCOMPARE(restored.rowCount(), source.length());
        for (int row = 0; row < restored.rowCount(); ++row)
            QCOMPARE(restored.index(row, 0).data(contentType::data).toMap(), source.index(row).data(contentType::data).toMap());
        QVariantMap restoredData;
        QVERIFY(deserializeData(&restoredData, serializeData(repeated)));
        QCOMPARE(restoredData, repeated);
#ifdef WITH_QCA_ENCRYPTION
        QVERIFY(Encryption::initialize());
        Encryption::EncryptionKey key;
        QVERIFY(key.generateRandomDEK());
        const auto encrypted = Encryption::encrypt(Encryption::SecureArray(serializeData(repeated)), key);
        QVERIFY(!encrypted.isEmpty());
        QVERIFY(encrypted != serializeData(repeated));
        QVariantMap decrypted;
        QVERIFY(deserializeData(&decrypted, Encryption::decrypt(encrypted, key)));
        QCOMPARE(decrypted, repeated);
#endif
    }

    void historyCaptureFilters()
    {
        AppConfig config;
        ConfigurationManager cli;
        QVERIFY(cli.setOptionValue(Config::clipboard_history_types::name(), QByteArray("text"), &config));
        QCOMPARE(config.option<Config::clipboard_history_types>(), QStringList{QStringLiteral("text")});
        config.setOption(Config::check_clipboard::name(), true);
        config.setOption(Config::clipboard_history_types::name(), QStringList{QStringLiteral("text")});
        config.setOption(Config::clipboard_history_ignore_apps::name(), QStringList{QStringLiteral("ignored.APP")});
        ClipboardMonitor monitor({mimeText, mimeHtml, mimeUriList, QStringLiteral("image/png"), mimeSecret});
        QString application = QStringLiteral("allowed.app");
        connect(&monitor, &ClipboardMonitor::fetchCurrentClipboardOwner, &monitor,
            [&application](QString *title, QString *identity) { *title = QStringLiteral("Window"); *identity = application; });
        monitor.setClipboardOwner({QStringLiteral("Window"), QStringLiteral("allowed.app")});
        QSignalSpy capture(&monitor, &ClipboardMonitor::clipboardChanged);
        QSignalSpy repeat(&monitor, &ClipboardMonitor::clipboardUnchanged);
        QSignalSpy secret(&monitor, &ClipboardMonitor::secretClipboardChanged);
        const auto copy = [&monitor](const QVariantMap &data) {
            auto mime = new QMimeData;
            for (auto it = data.cbegin(); it != data.cend(); ++it) mime->setData(it.key(), it.value().toByteArray());
            QApplication::clipboard()->setMimeData(mime);
            return QMetaObject::invokeMethod(&monitor, "onClipboardChanged", Qt::DirectConnection,
                Q_ARG(ClipboardMode, ClipboardMode::Clipboard));
        };
        const auto text = textData("captured");
        QVERIFY(copy(text));
        QCOMPARE(capture.size(), 1);
        const auto captured = capture.first().first().toMap();
        QVERIFY(HistoryPolicy::copiedAt(captured) >= 0);
        QCOMPARE(captured.value(mimeText).toByteArray(), QByteArray("captured"));
        QCOMPARE(captured.value(mimeSourceApplication).toByteArray(), QByteArray("allowed.app"));
        QVERIFY(copy(text));
        QCOMPARE(capture.size(), 1);
        QCOMPARE(repeat.size(), 1);
        application = QStringLiteral("IGNORED.app");
        monitor.setClipboardOwner({QStringLiteral("same title"), application});
        QVERIFY(copy(textData("ignored")));
        QCOMPARE(capture.size(), 1);
        QVERIFY(copy({{QStringLiteral("image/png"), QByteArray("image")}}));
        QCOMPARE(capture.size(), 1);
        auto sensitive = textData("secret");
        sensitive.insert(mimeSecret, QByteArray("1"));
        QVERIFY(copy(sensitive));
        QCOMPARE(capture.size(), 1);
        QCOMPARE(secret.size(), 1);
        QVERIFY(!HistoryPolicy::accepts(sensitive, Config::clipboard_history_types::defaultValue(), {}));
        QVERIFY(HistoryPolicy::accepts(textData("unknown source"), {QStringLiteral("text")}, {QStringLiteral("ignored.app")}));
        QVERIFY(!HistoryPolicy::accepts({{mimeUriList, QByteArray("file:///tmp/file")}, {mimeText, QByteArray("/tmp/file")}}, {QStringLiteral("text")}, {}));
    }

private:
    ClipboardBrowserSharedPtr m_shared;
    std::unique_ptr<ItemFactory> m_factory;
};

int main(int argc, char **argv)
{
    if (argc < 2 || qgetenv("COPYQ_SESSION_NAME") != "test" || qEnvironmentVariableIsEmpty("COPYQ_SETTINGS_PATH")
            || qEnvironmentVariableIsEmpty("COPYQ_ITEM_DATA_PATH") || qEnvironmentVariableIsEmpty("COPYQ_STATE_PATH")) {
        fprintf(stderr, "Use run-isolated.sh/ps1 and specify test functions.\n");
        return 2;
    }
    std::unique_ptr<QApplication> app(platformNativeInterface()->createServerApplication(argc, argv));
    initSession(app.get(), QStringLiteral("test"));
    const auto clipboard = backupClipboard();
    ManagementTests tests;
    const int result = QTest::qExec(&tests, argc, argv);
    restoreClipboard(clipboard);
    return result;
}

#include "tests_management.moc"
