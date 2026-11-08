#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

namespace luma::ui {

// Persists the N most-recently-opened document paths via QSettings (native,
// no new dependency). ImageDocument (P2.2) will call addPath() when a file
// actually opens; nothing calls it yet besides the drag-and-drop target.
class RecentFilesModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList paths READ paths NOTIFY pathsChanged)

public:
    explicit RecentFilesModel(QObject* parent = nullptr);

    QStringList paths() const;
    Q_INVOKABLE void addPath(const QString& path);
    Q_INVOKABLE void clear();

signals:
    void pathsChanged();

private:
    void load();
    void save();

    QStringList m_paths;
    static constexpr int kMaxEntries = 10;
};

} // namespace luma::ui
