#include <cstdio>
#include <cstring>

#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

#include "../ui/ImageDocument.h"
#include "../ui/PipelineController.h"

// PLAN.md P2.1 acceptance: `lumatune --version` must print and exit 0 with
// no QPA platform plugin available (e.g. a headless CI runner) - so this
// check runs before QGuiApplication ever constructs, since that
// construction is what requires a platform plugin.
namespace {
constexpr const char* kVersion = "0.1.0";

// P2.7/T-10: "Launch packaged app headless with --self-test: opens
// fixture, runs pipeline, exports, exits 0". Deliberately a plain
// QCoreApplication, not QGuiApplication/QQmlApplicationEngine - it
// drives ImageDocument/PipelineController directly, the same backends
// Main.qml binds to, so it needs no QPA platform plugin at all (not
// even "offscreen"), unlike the real GUI path below.
int runSelfTest(int argc, char* argv[], const QString& fixturePath) {
    QCoreApplication app(argc, argv);

    luma::ui::ImageDocument doc;
    if (!doc.openPath(fixturePath)) {
        std::fprintf(stderr, "self-test: %s\n", qPrintable(doc.errorString()));
        return 1;
    }

    luma::ui::PipelineController controller;
    controller.setDocument(&doc);

    QTemporaryDir tempDir;
    const QString outPath = tempDir.path() + QStringLiteral("/lumatune_selftest.png");

    QEventLoop loop;
    bool succeeded = false;
    QString failureMessage;
    QObject::connect(&controller, &luma::ui::PipelineController::exportSucceeded, &loop,
                      [&]() {
                          succeeded = true;
                          loop.quit();
                      });
    QObject::connect(&controller, &luma::ui::PipelineController::exportFailed, &loop,
                      [&](const QString& message) {
                          failureMessage = message;
                          loop.quit();
                      });
    // Guards against hanging CI forever if a worker thread deadlocks -
    // a real T-10 failure should fail fast, not time out the runner.
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);

    if (!controller.exportImage(outPath, 90, false, true)) {
        std::fprintf(stderr, "self-test: no document open to export\n");
        return 1;
    }
    loop.exec();

    if (!succeeded) {
        std::fprintf(stderr, "self-test: %s\n",
                      failureMessage.isEmpty() ? "export timed out" : qPrintable(failureMessage));
        return 1;
    }
    std::printf("self-test: ok (%s)\n", qPrintable(outPath));
    return 0;
}
} // namespace

int main(int argc, char* argv[]) {
    // A bare positional arg (AGENTS.md §4's documented
    // `lumatune tests/golden/fixtures/warm_cast.png` form) opens that
    // path at startup, via Main.qml's initialPath - no proper option
    // parsing yet since there's only ever been this one flag and this
    // one positional slot.
    QString initialPath;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("lumatune %s\n", kVersion);
            return 0;
        }
        if (std::strcmp(argv[i], "--self-test") == 0) {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "--self-test requires a fixture path\n");
                return 1;
            }
            return runSelfTest(argc, argv, QString::fromLocal8Bit(argv[i + 1]));
        }
        initialPath = QString::fromLocal8Bit(argv[i]);
    }

    // The native per-platform style (Windows' default) can't customize a
    // GroupBox's "label" sub-control (StageCard.qml's checkable group box),
    // only a non-native style can - same reason legacy/app.py forces Fusion
    // on the Widgets side.
    QQuickStyle::setStyle("Fusion");

    QGuiApplication app(argc, argv);
    // RecentFilesModel's QSettings calls need these to pick a stable
    // storage location (e.g. the registry key / plist domain).
    QCoreApplication::setOrganizationName("LumaTune");
    QCoreApplication::setApplicationName("LumaTune");

    QQmlApplicationEngine engine;
    if (!initialPath.isEmpty()) {
        engine.setInitialProperties({{"initialPath", initialPath}});
    }
    engine.loadFromModule("LumaTune", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
