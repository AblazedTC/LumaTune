#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>

#include "DiagnosticsModel.h"
#include "ImageDocument.h"
#include "PipelineController.h"

namespace {
QString goldenFixture(const QString& name) {
    return QString(LUMA_REPO_ROOT) + "/tests/golden/fixtures/" + name;
}
constexpr int kJobTimeoutMs = 5000; // generous for a debug/sanitizer CI build
} // namespace

// T-6 (PLAN.md §11, P2.4). DiagnosticsModel just republishes a real
// PipelineResult's Diagnostics (Diagnostics.h) as QML-friendly
// properties - these tests run the real pipeline (same pattern as
// PipelineControllerTest) rather than constructing a fake Diagnostics,
// since there's no public seam to hand it one directly.
class DiagnosticsModelTest : public QObject {
    Q_OBJECT
private slots:
    void populatesFromRealPreview();
};

void DiagnosticsModelTest::populatesFromRealPreview() {
    luma::ui::ImageDocument doc;
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));

    luma::ui::PipelineController controller;
    luma::ui::DiagnosticsModel diagnostics;
    diagnostics.setController(&controller);

    QSignalSpy diagSpy(&diagnostics, &luma::ui::DiagnosticsModel::diagnosticsChanged);
    controller.setDocument(&doc);
    QVERIFY(diagSpy.wait(kJobTimeoutMs));

    QVERIFY(!diagnostics.timings().isEmpty());
    QCOMPARE(diagnostics.totalMs() >= 0.0, true);
    // Default levels params (Params.h: blackPct 0.5, whitePct 99.5) on a
    // real decoded image always yield a black point strictly below the
    // white point on the 0-255 scale.
    QVERIFY(diagnostics.blackPoint() < diagnostics.whitePoint());
}

QTEST_GUILESS_MAIN(DiagnosticsModelTest)
#include "DiagnosticsModelTest.moc"
