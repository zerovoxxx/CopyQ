// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.h"
#include "test_utils.h"
#include "tests_common.h"
#include "platform/platformnativeinterface.h"
#include "platform/platformwindow.h"
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QSysInfo>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <qscopeguard.h>
#ifdef Q_OS_MACOS
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

namespace {
QJsonObject distribution(QList<double> values)
{
    std::sort(values.begin(), values.end());
    return {{"samples", values.size()}, {"p50_ms", values.isEmpty() ? QJsonValue() : values[(values.size()-1)/2]},
        {"p95_ms", values.isEmpty() ? QJsonValue() : values[qCeil(values.size()*0.95)-1]}};
}

QJsonObject processGroup(qint64 pid)
{
#ifdef Q_OS_UNIX
    QProcess ps;
    ps.start(QStringLiteral("ps"), {QStringLiteral("-axo"), QStringLiteral("pid=,ppid=,time=,rss=")});
    if (!ps.waitForFinished(5000) || ps.exitCode()) return {};
    struct Row { qint64 pid, parent, rss; double cpu; };
    QList<Row> rows;
    for (const auto &line : ps.readAllStandardOutput().split('\n')) {
        const auto fields = QString::fromLatin1(line).simplified().split(' ');
        if (fields.size() != 4) continue;
        const auto time = fields[2].split(':'); double seconds = 0;
        for (const auto &part : time) seconds = seconds * 60 + part.toDouble();
        rows.append({fields[0].toLongLong(), fields[1].toLongLong(), fields[3].toLongLong()*1024, seconds});
    }
    QSet<qint64> group{pid};
    bool changed;
    do { changed = false; for (const auto &row : rows) if (group.contains(row.parent) && !group.contains(row.pid)) { group.insert(row.pid); changed = true; } } while (changed);
    qint64 rss = 0; double cpu = 0;
    for (const auto &row : rows) if (group.contains(row.pid)) { rss += row.rss; cpu += row.cpu; }
    return {{"processes", group.size()}, {"rss_bytes", rss}, {"cpu_seconds", cpu}};
#else
    return {{"server_rss_bytes", platformNativeInterface()->processResidentMemoryBytes(pid)}, {"cpu_seconds", QJsonValue()}};
#endif
}
}

void CoreTests::desktopPerformance()
{
    const auto outputPath = qEnvironmentVariable("COPYQ_PERF_OUTPUT");
    if (outputPath.isEmpty()) QSKIP("Set COPYQ_PERF_OUTPUT to run the explicit 30-sample desktop benchmark.");
    const bool upstream = qEnvironmentVariableIsSet("COPYQ_PERF_UPSTREAM");
    m_test->setEnv(QStringLiteral("COPYQ_ALLOW_PLUGINS"),QStringLiteral("*"));
    TEST(m_test->stopServer()); TEST(m_test->startServer());
    QJsonObject report{{"application", upstream ? "CopyQ" : "QClip"}, {"os", QSysInfo::prettyProductName()},
        {"architecture", QSysInfo::currentCpuArchitecture()}, {"qt", qVersion()}, {"samples", 30},
        {"display", qEnvironmentVariable("DISPLAY")}, {"qpa", qEnvironmentVariable("QT_QPA_PLATFORM")},
        {"render_loop", qEnvironmentVariable("QSG_RENDER_LOOP")}, {"method", "End-to-end CLI request through exposed UI/result; polling and IPC included"}};
    const auto writeReport = qScopeGuard([&] { QSaveFile file(outputPath); if (file.open(QIODevice::WriteOnly)) { file.write(QJsonDocument(report).toJson()); file.commit(); } });
    const auto client = [&](const QStringList &args) { QByteArray value; const auto error = m_test->getClientOutput(args, &value); if (!error.isEmpty()) m_test->writeOutErrors(error); return error.isEmpty() ? value : QByteArray("CLIENT_ERROR"); };
    const auto eval = [&](const QString &script) { return client({QStringLiteral("eval"), script}); };
    const auto wait = [&](const std::function<bool()> &ready) { QElapsedTimer timer; timer.start(); while (!ready()) { if (timer.elapsed() > 10000) return false; QTest::qWait(5); } return true; };
    const auto state = [&] { return QJsonDocument::fromJson(eval(QStringLiteral("var s=callPlugin('itemtests','%1'); JSON.stringify({visible:s.visible,exposed:s.exposed,filtering:s.filtering,count:Number(str(s.count)),query:str(s.query)})").arg(upstream ? QStringLiteral("desktopState") : QStringLiteral("paletteState")))).object(); };
    const auto hide = [&] { client({"hide"}); if(!upstream) eval(QStringLiteral("callPlugin('itemtests','paletteInput','Escape')")); };
    const auto show = [&] { if (!upstream && state().value("visible").toBool()) return true; client({upstream ? QStringLiteral("show") : QStringLiteral("palette")}); return wait([&] { const auto s = state(); return upstream ? s.value("exposed").toBool() : s.value("visible").toBool() && !s.value("filtering").toBool(); }); };
    RUN("config" << "maxitems" << "11000", "11000\n");
    RUN("config" << "activate_pastes" << "true", "true\n");
    RUN("config" << "activate_closes" << "true", "true\n");
    RUN("config" << "clipboard_tab" << "PERF", "PERF\n");
    hide();
    report.insert("runtime_info",QString::fromUtf8(client({"--info"})));
    QTemporaryDir temp;
    const auto imagePath = temp.filePath(QStringLiteral("sample.png"));
    QImage image(256,256,QImage::Format_ARGB32); image.fill(QColor(32,64,128)); QVERIFY(image.save(imagePath));
    QJsonArray datasets;
    for (const int count : {1000,10000}) {
        client({"disable"});
        eval(QStringLiteral("if(tab().indexOf('PERF')>=0) removeTab('PERF'); tab('PERF'); var a=[]; for(var i=0;i<%1;i++) a.push('text '+i+' 中文 payload '+('00000'+i).slice(-5)); add.apply(this,a); setCurrentTab('PERF');").arg(count));
        QCOMPARE(eval(QStringLiteral("tab('PERF'); size()" )).trimmed().toInt(), count);
        for (int i=0; i<5; ++i) client({"eval", QStringLiteral("tab('PERF'); var f=new File(%2); f.openReadOnly(); write(%1,'image/png',f.readAll()); f.close();").arg(i).arg(QString::fromUtf8(QJsonDocument(QJsonArray{imagePath}).toJson(QJsonDocument::Compact)).mid(1).chopped(1))});
        QVERIFY(show());
        eval(QStringLiteral("callPlugin('itemtests','frameTimes')"));
        QList<double> opens, searches, captures, pastes, cold;
        QJsonObject dataset{{"text_items",count}, {"images",5}, {"image_dimensions","256x256 RGBA; five fixed leading items"}};
        const auto sampleCpu = [&](bool visible) {
            if (visible) show(); else hide();
            QTest::qWait(500);
            const auto before = processGroup(m_test->serverPid());
            QElapsedTimer timer; timer.start(); QTest::qWait(3000);
            const auto after = processGroup(m_test->serverPid());
            QJsonObject sample = after;
            if (before.contains("cpu_seconds") && before.value("cpu_seconds").isDouble()) sample.insert("cpu_percent", 100000.0*(after.value("cpu_seconds").toDouble()-before.value("cpu_seconds").toDouble())/timer.elapsed());
            sample.remove("cpu_seconds"); return sample;
        };
        dataset.insert("hidden_process_group", sampleCpu(false));
        dataset.insert("visible_process_group", sampleCpu(true));
        for (int sample=0; sample<30; ++sample) {
            hide(); QTest::qWait(20);
            QElapsedTimer timer; timer.start(); QVERIFY(show()); opens.append(timer.nsecsElapsed()/1000000.0);
            const auto query = QStringLiteral("payload %1").arg(sample+20,5,10,QLatin1Char('0'));
            timer.restart();
            if (upstream) client({"filter",query});
            else eval(QStringLiteral("callPlugin('itemtests','paletteInput','Ctrl+A'); callPlugin('itemtests','paletteInput','commit:%1')").arg(query));
            QVERIFY2(wait([&] { const auto s=state(); return s.value("count").toInt()==1 && (upstream || !s.value("filtering").toBool()); }),QJsonDocument(state()).toJson().constData());
            searches.append(timer.nsecsElapsed()/1000000.0);
            hide(); client({"enable"});
            const auto text = QStringLiteral("capture-%1-%2").arg(count).arg(sample).toUtf8();
            timer.restart(); TEST(m_test->setClipboard(text));
            QVERIFY(wait([&] { return client({"tab","PERF","read","0"}) == text; })); captures.append(timer.nsecsElapsed()/1000000.0);
            client({"disable"});
            // The native receiver verifies the actual target text, not clipboard ownership alone.
            bool native = true;
#ifdef Q_OS_MACOS
            native = AXIsProcessTrusted() && !IsSecureEventInputEnabled();
#endif
            if (native) {
                const auto target = temp.filePath(QStringLiteral("target.txt")); QFile::remove(target);
                QProcess receiver;
                auto helper = QCoreApplication::applicationDirPath()+QStringLiteral("/copyq-palette-tests");
#ifdef Q_OS_WIN
                helper += QStringLiteral(".exe");
#endif
                receiver.start(helper,{"--input-target",target}); QVERIFY(receiver.waitForStarted());
                const auto cleanup=qScopeGuard([&] { receiver.terminate(); if (!receiver.waitForFinished(1000)) { receiver.kill(); receiver.waitForFinished(1000); } });
                QVERIFY(wait([&] { const auto w=platformNativeInterface()->getCurrentWindow(); return w && w->isActive() && w->getTitle() == QStringLiteral("QClip input target"); }));
                QTest::qWait(100);
                if (upstream) client({"filter",""});
                QVERIFY(show());
                if (!upstream) eval(QStringLiteral("callPlugin('itemtests','paletteInput','Ctrl+A'); callPlugin('itemtests','paletteInput','Backspace')"));
                QVERIFY(wait([&] { return upstream || !state().value("filtering").toBool(); }));
                if (!upstream) eval(QStringLiteral("callPlugin('itemtests','paletteSelect',0)"));
                timer.restart();
                eval(upstream ? QStringLiteral("plugins.itemtests.keys('ENTER')") : QStringLiteral("callPlugin('itemtests','paletteInput','Return')"));
                const bool received=wait([&] { QFile file(target); return file.open(QIODevice::ReadOnly) && file.readAll().contains(text); });
                QFile actual(target); actual.open(QIODevice::ReadOnly);
                QVERIFY2(received, (QByteArray("Target: ")+actual.readAll()+"; expected: "+text+"; state: "+QJsonDocument(state()).toJson()).constData());
                pastes.append(timer.nsecsElapsed()/1000000.0);
            }
        }
        const auto frames=QJsonDocument::fromJson(eval(QStringLiteral("JSON.stringify(callPlugin('itemtests','frameTimes').map(function(v){return Number(str(v))}))"))).array();
        QList<double> render; for (const auto frame : frames) render.append(frame.toDouble()/1000.0);
        dataset.insert("render_cpu_time",distribution(render));
        hide();
        for(int sample=0;sample<30;++sample) {
            TEST(m_test->stopServer()); QElapsedTimer timer; timer.start(); TEST(m_test->startServer());
            eval(QStringLiteral("setCurrentTab('PERF')")); QVERIFY(show());
            QCOMPARE(eval(QStringLiteral("tab('PERF'); size()" )).trimmed().toInt(),count+35);
            cold.append(timer.nsecsElapsed()/1000000.0); hide();
        }
        dataset.insert("resident_show",distribution(opens)); dataset.insert("search",distribution(searches));
        dataset.insert("clipboard_capture",distribution(captures)); dataset.insert("native_paste",distribution(pastes)); dataset.insert("cold_start_and_show",distribution(cold));
        bool passed=true;
        for (const auto pair : {qMakePair(distribution(opens),250.0),qMakePair(distribution(searches),200.0),qMakePair(distribution(captures),1000.0),qMakePair(distribution(pastes),1000.0),qMakePair(distribution(cold),3000.0)})
            passed = passed && pair.first.value("samples").toInt()==30 && pair.first.value("p95_ms").toDouble()<=pair.second;
        dataset.insert("frozen_thresholds_passed",passed); datasets.append(dataset); report.insert("datasets",datasets);
    }
    if (!upstream)
        for(const auto dataset : datasets) QVERIFY2(dataset.toObject().value("frozen_thresholds_passed").toBool(), qPrintable(outputPath+QStringLiteral(": frozen latency thresholds failed or native samples missing")));
}
