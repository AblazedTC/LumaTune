#include "ImageDocument.h"

#include "../core/io/Decoder.h"

namespace luma::ui {

namespace {

QString toMessage(luma::IoError error, const QString& path) {
    switch (error) {
        case luma::IoError::FileNotFound:
            return QStringLiteral("File not found: %1").arg(path);
        case luma::IoError::DecodeFailed:
            return QStringLiteral("Could not decode image: %1").arg(path);
        case luma::IoError::EncodeFailed:
            break; // decodeImage() never returns this; no case produces it here
    }
    return QStringLiteral("Unknown error loading: %1").arg(path);
}

} // namespace

ImageDocument::ImageDocument(QObject* parent) : QObject(parent) {}

bool ImageDocument::open(const QUrl& url) {
    const QString local = url.toLocalFile();
    if (local.isEmpty()) {
        m_errorString = QStringLiteral("Not a local file: %1").arg(url.toString());
        emit errorOccurred(m_errorString);
        emit errorStringChanged();
        return false;
    }
    return openPath(local);
}

bool ImageDocument::openPath(const QString& path) {
    auto result = luma::decodeImage(path.toStdString());
    if (!result) {
        m_errorString = toMessage(result.error(), path);
        emit errorOccurred(m_errorString);
        emit errorStringChanged();
        return false;
    }
    m_buffer = std::move(result.value());
    m_path = path;
    if (!m_errorString.isEmpty()) {
        m_errorString.clear();
        emit errorStringChanged();
    }
    emit pathChanged();
    return true;
}

} // namespace luma::ui
