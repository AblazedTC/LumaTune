#pragma once

#include <memory>

#include <QImage>
#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>

#include "../core/engine/Pipeline.h"
#include "PipelineController.h"

namespace luma::ui {

// Renders PipelineController's latest preview result (PLAN.md §8.1's
// Viewer.qml/PreviewItem row). Texture upload happens in
// updatePaintNode() on the render thread, as PLAN.md §8.2 specifies;
// previewReady() (emitted on the UI/GUI thread - PipelineController.cpp's
// own Qt::QueuedConnection hop already landed it there) just stashes the
// result and calls update(). No extra locking is needed between the two:
// the threaded render loop blocks the GUI thread for the whole sync phase
// that updatePaintNode runs in, so m_pending can't be written and read at
// the same time (ordinary Qt Quick custom-item threading, not something
// specific to this class).
class PreviewItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(luma::ui::PipelineController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)

public:
    explicit PreviewItem(QQuickItem* parent = nullptr);

    PipelineController* controller() const { return m_controller; }
    void setController(PipelineController* controller);

signals:
    void controllerChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void onPreviewReady(std::shared_ptr<const luma::PipelineResult> result);

    QPointer<PipelineController> m_controller;
    QMetaObject::Connection m_readyConnection;
    // Set on the GUI thread by onPreviewReady(), consumed on the render
    // thread by updatePaintNode() - see the class comment above for why
    // that's safe without a mutex.
    std::shared_ptr<const luma::PipelineResult> m_pending;
};

// RGB_F32 (sRGB-encoded, [0,1] - Pipeline.h's PipelineResult contract) to
// QImage::Format_RGBA16FPx4, alpha always opaque. Pulled out as a free
// function so it's unit-testable without a QQuickWindow/GPU context
// (updatePaintNode's actual QSGTexture upload is exercised by launching
// the app instead - AGENTS.md §4's "ctest -L ui and launch the app once
// with a fixture" for src/ui/** changes).
QImage toRgba16FImage(const luma::ImageBuffer& buffer);

} // namespace luma::ui
