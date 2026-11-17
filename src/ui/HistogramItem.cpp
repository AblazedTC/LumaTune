#include "HistogramItem.h"

#include <algorithm>
#include <utility>

#include <QColor>
#include <QPainter>
#include <QPainterPath>

namespace luma::ui {

HistogramItem::HistogramItem(QQuickItem* parent) : QQuickPaintedItem(parent) {}

void HistogramItem::setController(PipelineController* controller) {
    if (m_controller == controller) {
        return;
    }
    QObject::disconnect(m_readyConnection);
    m_controller = controller;
    if (m_controller) {
        m_readyConnection = connect(m_controller, &PipelineController::previewReady, this,
                                    &HistogramItem::onPreviewReady);
    }
    emit controllerChanged();
}

void HistogramItem::onPreviewReady(std::shared_ptr<const luma::PipelineResult> result) {
    m_before = result->diag.lumaBefore;
    m_after = result->diag.lumaAfter;
    m_hasData = true;
    update();
}

void HistogramItem::paint(QPainter* painter) {
    painter->fillRect(boundingRect(), QColor(20, 20, 20));
    if (!m_hasData) {
        return;
    }

    const float maxBefore = *std::max_element(m_before.begin(), m_before.end());
    const float maxAfter = *std::max_element(m_after.begin(), m_after.end());
    const float maxCount = std::max({maxBefore, maxAfter, 1.0f}); // avoid a /0 on an empty image

    const qreal w = width();
    const qreal h = height();
    const qreal binWidth = w / static_cast<qreal>(m_before.size());

    auto drawHistogram = [&](const luma::Histogram256& hist, const QColor& color) {
        QPainterPath path;
        path.moveTo(0, h);
        for (std::size_t i = 0; i < hist.size(); ++i) {
            const qreal x = static_cast<qreal>(i) * binWidth;
            const qreal y = h - (hist[i] / maxCount) * h;
            path.lineTo(x, y);
        }
        path.lineTo(w, h);
        path.closeSubpath();
        painter->setBrush(color);
        painter->drawPath(path);
    };

    painter->setPen(Qt::NoPen);
    painter->setOpacity(0.6);
    drawHistogram(m_before, QColor(120, 120, 255)); // before AutoLevels/CLAHE - cool tint
    drawHistogram(m_after, QColor(255, 180, 60));   // after - warm tint
}

} // namespace luma::ui
