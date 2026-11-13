#include <cstdio>
#include <cstring>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QUrl>
#include <QVariantMap>

// PLAN.md P2.1 acceptance: `lumatune --version` must print and exit 0 with
// no QPA platform plugin available (e.g. a headless CI runner) - so this
// check runs before QGuiApplication ever constructs, since that
// construction is what requires a platform plugin.
namespace {
constexpr const char* kVersion = "0.1.0";
}

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
        initialPath = QString::fromLocal8Bit(argv[i]);
    }

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
