#pragma once

#include <memory>

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>

#include "../core/engine/Diagnostics.h"
#include "../core/engine/Pipeline.h"
#include "PipelineController.h"

namespace luma::ui {

// Per-stage readout backing a diagnostics panel (PLAN.md §8.1, P2.4:
// FR-10). Connects directly to PipelineController::previewReady - same
// pattern as PreviewItem - and republishes luma::Diagnostics' fields as
// QML-friendly properties. No RGB histogram/clipping/log-toggle state
// lives here (that's HistogramItem/P3.6); this phase only needs the
// Levels/CLAHE/WB readouts and per-stage timings FR-10 asks for.
class DiagnosticsModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(luma::ui::PipelineController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)
    Q_PROPERTY(QVariantList timings READ timings NOTIFY diagnosticsChanged)
    Q_PROPERTY(double totalMs READ totalMs NOTIFY diagnosticsChanged)
    Q_PROPERTY(bool anyGpu READ anyGpu NOTIFY diagnosticsChanged)
    // FR-10 asks for both representations - LevelsDiag.h carries both.
    Q_PROPERTY(float blackPoint READ blackPoint NOTIFY diagnosticsChanged) // 0-255 scale
    Q_PROPERTY(float whitePoint READ whitePoint NOTIFY diagnosticsChanged) // 0-255 scale
    Q_PROPERTY(float blackPct READ blackPct NOTIFY diagnosticsChanged)     // percentile
    Q_PROPERTY(float whitePct READ whitePct NOTIFY diagnosticsChanged)    // percentile
    Q_PROPERTY(QString wbExplanation READ wbExplanation NOTIFY diagnosticsChanged)
    Q_PROPERTY(float wbConfidence READ wbConfidence NOTIFY diagnosticsChanged)

public:
    explicit DiagnosticsModel(QObject* parent = nullptr);

    PipelineController* controller() const { return m_controller; }
    void setController(PipelineController* controller);

    QVariantList timings() const { return m_timings; }
    double totalMs() const { return m_totalMs; }
    bool anyGpu() const { return m_anyGpu; }
    float blackPoint() const { return m_diag.levels.blackPt; }
    float whitePoint() const { return m_diag.levels.whitePt; }
    float blackPct() const { return m_diag.levels.blackPct; }
    float whitePct() const { return m_diag.levels.whitePct; }
    QString wbExplanation() const { return QString::fromStdString(m_diag.wb.explanation); }
    float wbConfidence() const { return m_diag.wb.confidence; }

signals:
    void controllerChanged();
    void diagnosticsChanged();

private:
    void onPreviewReady(std::shared_ptr<const luma::PipelineResult> result);

    QPointer<PipelineController> m_controller;
    QMetaObject::Connection m_readyConnection;
    luma::Diagnostics m_diag;
    QVariantList m_timings;
    double m_totalMs{0.0};
    bool m_anyGpu{false};
};

} // namespace luma::ui
