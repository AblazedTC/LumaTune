#pragma once

#include <optional>

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include "../core/image/ImageBuffer.h"

namespace luma::ui {

// Wraps a decoded luma::ImageBuffer for QML (PLAN.md §8.1's component
// table: Viewer.qml is "backed by" ImageDocument). Decoding runs
// synchronously on open() - legacy/ui/main_window.py's _open_image never
// threaded ImageState.load() either, only Pipeline.run() went to
// QThreadPool (see git history/AGENTS.md §10). Revisit once a RAW
// decoder (V2, LibRaw) makes that cost large enough to matter.
class ImageDocument : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY pathChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorOccurred)

public:
    explicit ImageDocument(QObject* parent = nullptr);

    QString path() const { return m_path; }
    bool loaded() const { return m_buffer.has_value(); }
    QString errorString() const { return m_errorString; }

    // Accepts a file:// URL (drag-and-drop, FileDialog) or a plain local
    // path either way; non-local URLs fail with errorOccurred().
    Q_INVOKABLE bool open(const QUrl& url);
    Q_INVOKABLE bool openPath(const QString& path);

    // C++-only: PipelineController (same translation unit's worth of
    // trust - both are luma::ui backends) reads this to submit pipeline
    // jobs. luma::ImageBuffer isn't a QML-representable type, so this is
    // deliberately not a Q_PROPERTY/Q_INVOKABLE.
    const luma::ImageBuffer* buffer() const { return m_buffer ? &*m_buffer : nullptr; }

signals:
    void pathChanged();
    void errorOccurred(const QString& message);

private:
    QString m_path;
    QString m_errorString;
    std::optional<luma::ImageBuffer> m_buffer;
};

} // namespace luma::ui
