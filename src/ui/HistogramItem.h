#pragma once

#include <memory>

#include <QMetaObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickPaintedItem>

#include "../core/engine/Diagnostics.h"
#include "../core/engine/Pipeline.h"
#include "PipelineController.h"

namespace luma::ui {

// Custom-painted luma histogram (PLAN.md §8.1; AGENTS.md §8: Qt
// Charts/Graphs are GPL, forbidden - hand-rolled QQuickPaintedItem
// instead). P2.4's scope is luma-only (L channel, before/after
// AutoLevels+CLAHE+Curve+Saturation); RGB overlay, clipping indicators,
// and the log-scale toggle are P3.6.
//
// QQuickPaintedItem (unlike PreviewItem's plain QQuickItem) manages its
// own backing image/texture internally and always has something valid
// to paint, even before the first result arrives - no PreviewItem-style
// "textureless node" hazard here (PreviewItem.cpp's own comment covers
// that one).
class HistogramItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(luma::ui::PipelineController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)

public:
    explicit HistogramItem(QQuickItem* parent = nullptr);

    PipelineController* controller() const { return m_controller; }
    void setController(PipelineController* controller);

    void paint(QPainter* painter) override;

signals:
    void controllerChanged();

private:
    void onPreviewReady(std::shared_ptr<const luma::PipelineResult> result);

    QPointer<PipelineController> m_controller;
    QMetaObject::Connection m_readyConnection;
    luma::Histogram256 m_before{};
    luma::Histogram256 m_after{};
    bool m_hasData{false};
};

} // namespace luma::ui
