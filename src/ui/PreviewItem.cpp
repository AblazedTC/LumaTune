#include "PreviewItem.h"

#include <utility>

#include <QFloat16>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>

#include <opencv2/core.hpp>

namespace luma::ui {

QImage toRgba16FImage(const luma::ImageBuffer& buffer) {
    const cv::Mat src = buffer.mat(); // CV_32FC3, RGB order (AGENTS.md §6)
    QImage image(buffer.width, buffer.height, QImage::Format_RGBA16FPx4);
    for (int y = 0; y < buffer.height; ++y) {
        const float* srcRow = src.ptr<float>(y);
        auto* dstRow = reinterpret_cast<qfloat16*>(image.scanLine(y));
        for (int x = 0; x < buffer.width; ++x) {
            dstRow[x * 4 + 0] = qfloat16(srcRow[x * 3 + 0]);
            dstRow[x * 4 + 1] = qfloat16(srcRow[x * 3 + 1]);
            dstRow[x * 4 + 2] = qfloat16(srcRow[x * 3 + 2]);
            dstRow[x * 4 + 3] = qfloat16(1.0f);
        }
    }
    return image;
}

PreviewItem::PreviewItem(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

void PreviewItem::setController(PipelineController* controller) {
    if (m_controller == controller) {
        return;
    }
    QObject::disconnect(m_readyConnection);
    m_controller = controller;
    if (m_controller) {
        m_readyConnection = connect(m_controller, &PipelineController::previewReady, this,
                                     &PreviewItem::onPreviewReady);
    }
    emit controllerChanged();
}

void PreviewItem::onPreviewReady(std::shared_ptr<const luma::PipelineResult> result) {
    m_pending = std::move(result);
    update();
}

void PreviewItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    update();
}

QSGNode* PreviewItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    auto* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node) {
        // No document has been opened yet (the common first frame - no
        // CLI path, nothing dropped, nothing from recents) - a
        // QSGSimpleTextureNode with no texture ever set on it crashes
        // the scenegraph renderer (a real CI segfault caught by the
        // Linux smoke test on PR #21), so there's nothing to add to the
        // tree until the first result actually arrives.
        if (!m_pending) {
            return nullptr;
        }
        node = new QSGSimpleTextureNode();
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Linear);
    }
    if (m_pending) {
        const QImage image = toRgba16FImage(m_pending->image);
        node->setTexture(window()->createTextureFromImage(image));
        m_pending.reset();
    }
    node->setRect(boundingRect());
    return node;
}

} // namespace luma::ui
