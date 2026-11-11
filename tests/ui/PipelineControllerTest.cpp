#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>

#include "ImageDocument.h"
#include "PipelineController.h"

namespace {
QString goldenFixture(const QString& name) {
    return QString(LUMA_REPO_ROOT) + "/tests/golden/fixtures/" + name;
}
constexpr int kJobTimeoutMs = 5000; // generous for a debug/sanitizer CI build
} // namespace

// T-6 (PLAN.md §11). PipelineController's jobs run on JobScheduler's
// worker threads and land back on this (the test's) thread via
// QMetaObject::invokeMethod(..., Qt::QueuedConnection) - QSignalSpy::wait()
// pumps this thread's event loop, which is what actually lets that queued
// call (and therefore the signal it emits) be delivered.
class PipelineControllerTest : public QObject {
    Q_OBJECT
private slots:
    void settingDocumentSubmitsPreview();
    void changingParamSubmitsNewPreview();
    void rapidParamChangesDeliverOnlyTheLast();
    void noDocumentNeverSubmits();
};

void PipelineControllerTest::settingDocumentSubmitsPreview() {
    luma::ui::ImageDocument doc;
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);
    QSignalSpy failedSpy(&controller, &luma::ui::PipelineController::previewFailed);

    controller.setDocument(&doc);

    QVERIFY(readySpy.wait(kJobTimeoutMs));
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(failedSpy.count(), 0);
    QVERIFY(!controller.busy());

    const auto result =
        readySpy.at(0).at(0).value<std::shared_ptr<const luma::PipelineResult>>();
    QVERIFY(result != nullptr);
    QVERIFY(result->image.width > 0);
    QVERIFY(result->image.height > 0);
}

void PipelineControllerTest::changingParamSubmitsNewPreview() {
    luma::ui::ImageDocument doc;
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    controller.setDocument(&doc);

    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);
    QVERIFY(readySpy.wait(kJobTimeoutMs));
    readySpy.clear();

    controller.setBlackPct(1.5f); // default is 0.5 (Params.h) - an actual change
    QVERIFY(readySpy.wait(kJobTimeoutMs));
    QCOMPARE(readySpy.count(), 1);
    QVERIFY(!controller.busy());
}

void PipelineControllerTest::rapidParamChangesDeliverOnlyTheLast() {
    luma::ui::ImageDocument doc;
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    controller.setDocument(&doc);

    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);
    QVERIFY(readySpy.wait(kJobTimeoutMs));
    readySpy.clear();

    // JobScheduler's latest-only PREVIEW slot (JobScheduler.h) guarantees
    // at most one of these three ever reaches previewReady - whichever
    // one is still the slot's occupant when it finishes.
    controller.setBlackPct(1.0f);
    controller.setWhitePct(98.0f);
    controller.setClaheClip(3.0f);

    QVERIFY(readySpy.wait(kJobTimeoutMs));
    QCOMPARE(readySpy.count(), 1);
    QVERIFY(!controller.busy());
}

void PipelineControllerTest::noDocumentNeverSubmits() {
    luma::ui::PipelineController controller;
    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);

    controller.setBlackPct(2.0f);

    QVERIFY(!readySpy.wait(200)); // no document buffer to run against - nothing to wait for
    QCOMPARE(readySpy.count(), 0);
}

QTEST_GUILESS_MAIN(PipelineControllerTest)
#include "PipelineControllerTest.moc"
