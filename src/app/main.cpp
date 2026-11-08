#include <cstdio>
#include <cstring>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

// PLAN.md P2.1 acceptance: `lumatune --version` must print and exit 0 with
// no QPA platform plugin available (e.g. a headless CI runner) - so this
// check runs before QGuiApplication ever constructs, since that
// construction is what requires a platform plugin.
namespace {
constexpr const char* kVersion = "0.1.0";
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("lumatune %s\n", kVersion);
            return 0;
        }
    }

    QGuiApplication app(argc, argv);
    // RecentFilesModel's QSettings calls need these to pick a stable
    // storage location (e.g. the registry key / plist domain).
    QCoreApplication::setOrganizationName("LumaTune");
    QCoreApplication::setApplicationName("LumaTune");

    QQmlApplicationEngine engine;
    engine.loadFromModule("LumaTune", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
