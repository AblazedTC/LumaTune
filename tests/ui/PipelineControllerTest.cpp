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
    void documentDestructionIsSafe();
    void switchingDocumentDropsStalePreview();
    void reentrantBusyListenerDoesNotStickBusy();
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

void PipelineControllerTest::documentDestructionIsSafe() {
    // CodeRabbit review on PR #20: m_document used to be a raw pointer -
    // destroying the document out from under the controller (QML can own
    // both independently) would leave it dangling, and the next setter
    // call would dereference it via submitPreview(). QPointer fixes this;
    // this test is the regression guard for that fix.
    auto* doc = new luma::ui::ImageDocument();
    QVERIFY(doc->openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);
    controller.setDocument(doc);
    QVERIFY(readySpy.wait(kJobTimeoutMs));
    readySpy.clear();

    delete doc;
    QCOMPARE(controller.document(), nullptr);

    controller.setBlackPct(3.0f); // must not dereference the now-dangling pointer
    QVERIFY(!readySpy.wait(200)); // no buffer to run against any more
    QCOMPARE(readySpy.count(), 0);
}

void PipelineControllerTest::switchingDocumentDropsStalePreview() {
    // CodeRabbit review on PR #20: JobScheduler's latest-only PREVIEW slot
    // only dedups jobs for the *same* DocumentId - switching to a second
    // document starts an independent job that JobScheduler has no reason
    // to consider stale. Both docA's and docB's jobs genuinely run; only
    // docB's (the one still current when its result arrives) must ever
    // reach previewReady.
    luma::ui::ImageDocument docA;
    QVERIFY(docA.openPath(goldenFixture("neutral.png")));
    luma::ui::ImageDocument docB;
    QVERIFY(docB.openPath(goldenFixture("warm_cast.jpg")));

    luma::ui::PipelineController controller;
    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);

    controller.setDocument(&docA);
    controller.setDocument(&docB); // switches before docA's job can plausibly have finished

    QVERIFY(readySpy.wait(kJobTimeoutMs));
    QTest::qWait(50); // give docA's result a chance to (incorrectly) arrive too, if it's going to
    QCOMPARE(readySpy.count(), 1);
    QVERIFY(!controller.busy());

    const auto result =
        readySpy.at(0).at(0).value<std::shared_ptr<const luma::PipelineResult>>();
    QVERIFY(result != nullptr);
}

void PipelineControllerTest::reentrantBusyListenerDoesNotStickBusy() {
    // CodeRabbit review on PR #20 (third pass): submitPreview() used to
    // call setBusy(true) *before* its own scheduler.submit(). A
    // same-thread busyChanged() listener that reacts by calling another
    // setter would get its own (newer) job submitted first, then get
    // evicted by this call's own (older) job once it finally reached its
    // own submit() - permanently desyncing the seq check that clears
    // busy(). Calling submit() before setBusy(true) fixes the ordering.
    luma::ui::ImageDocument doc;
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    controller.setDocument(&doc);
    QSignalSpy readySpy(&controller, &luma::ui::PipelineController::previewReady);
    QVERIFY(readySpy.wait(kJobTimeoutMs));
    readySpy.clear();

    bool reentered = false;
    QObject::connect(&controller, &luma::ui::PipelineController::busyChanged, [&]() {
        if (controller.busy() && !reentered) {
            reentered = true;
            controller.setWhitePct(97.0f); // reentrant setter, called from inside busyChanged()
        }
    });

    controller.setBlackPct(2.5f);
    QVERIFY(reentered);

    QVERIFY(readySpy.wait(kJobTimeoutMs));
    QCOMPARE(readySpy.count(), 1);
    QVERIFY(!controller.busy());
}

QTEST_GUILESS_MAIN(PipelineControllerTest)
#include "PipelineControllerTest.moc"
