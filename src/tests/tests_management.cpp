// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app.h"
#include "common/appconfig.h"
#include "common/common.h"
#include "common/config.h"
#include "common/contenttype.h"
#include "common/mimetypes.h"
#include "common/settings.h"
#include "gui/clipboardbrowser.h"
#include "gui/clipboardbrowsershared.h"
#include "gui/clipboardmanagement.h"
#include "gui/clipboardpalette.h"
#include "item/clipboardmodel.h"
#include "item/itemfactory.h"
#include "platform/platformnativeinterface.h"
#include "tests/clipboardguard.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QInputMethodEvent>
#include <QMenu>
#include <QMimeData>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardItemModel>
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
        model.selectAll();
        auto item = PersistentDisplayItem(&model, model.selectedIndex(), model.displayRevision(), model.selectedData());
        item.setData(textData("displayed"));
        QCOMPARE(model.preview().value(QStringLiteral("text")).toString(), QStringLiteral("displayed"));
        QCOMPARE(source->index(1).data(contentType::data).toMap(), textData("two"));
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
