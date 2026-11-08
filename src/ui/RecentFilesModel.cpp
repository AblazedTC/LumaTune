#include "RecentFilesModel.h"

#include <QSettings>

namespace luma::ui {

namespace {
constexpr const char* kSettingsKey = "recentFiles";
}

RecentFilesModel::RecentFilesModel(QObject* parent) : QObject(parent) {
    load();
}

QStringList RecentFilesModel::paths() const {
    return m_paths;
}

void RecentFilesModel::addPath(const QString& path) {
    m_paths.removeAll(path);
    m_paths.prepend(path);
    while (m_paths.size() > kMaxEntries) {
        m_paths.removeLast();
    }
    save();
    emit pathsChanged();
}

void RecentFilesModel::clear() {
    m_paths.clear();
    save();
    emit pathsChanged();
}

void RecentFilesModel::load() {
    QSettings settings;
    m_paths = settings.value(kSettingsKey).toStringList();
}

void RecentFilesModel::save() {
    QSettings settings;
    settings.setValue(kSettingsKey, m_paths);
}

} // namespace luma::ui
