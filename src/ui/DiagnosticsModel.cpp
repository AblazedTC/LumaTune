#include "DiagnosticsModel.h"

#include <utility>

namespace luma::ui {

DiagnosticsModel::DiagnosticsModel(QObject* parent) : QObject(parent) {}

void DiagnosticsModel::setController(PipelineController* controller) {
    if (m_controller == controller) {
        return;
    }
    QObject::disconnect(m_readyConnection);
    m_controller = controller;
    if (m_controller) {
        m_readyConnection = connect(m_controller, &PipelineController::previewReady, this,
                                    &DiagnosticsModel::onPreviewReady);
    }
    emit controllerChanged();
}

void DiagnosticsModel::onPreviewReady(std::shared_ptr<const luma::PipelineResult> result) {
    m_diag = result->diag;
    m_timings.clear();
    m_totalMs = 0.0;
    m_anyGpu = false;
    for (const auto& timing : m_diag.timings) {
        QVariantMap entry;
        entry[QStringLiteral("stage")] = QString::fromStdString(timing.stage);
        entry[QStringLiteral("ms")] = timing.ms;
        entry[QStringLiteral("onGpu")] = timing.onGpu;
        m_timings.append(entry);
        m_totalMs += timing.ms;
        m_anyGpu = m_anyGpu || timing.onGpu;
    }
    emit diagnosticsChanged();
}

} // namespace luma::ui
