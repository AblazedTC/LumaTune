#include "PipelineController.h"

#include <cstdint>
#include <utility>

#include "../core/engine/PreviewPolicy.h"

namespace luma::ui {

PipelineController::PipelineController(QObject* parent) : QObject(parent) {
    qRegisterMetaType<std::shared_ptr<const luma::PipelineResult>>();
}

PipelineController::~PipelineController() = default;

void PipelineController::setDocument(ImageDocument* document) {
    if (m_document == document) {
        return;
    }
    QObject::disconnect(m_documentConnection);
    m_document = document;
    if (m_document) {
        m_documentConnection = connect(m_document, &ImageDocument::pathChanged, this,
                                        &PipelineController::submitPreview);
    }
    emit documentChanged();
    if (m_document && m_document->buffer()) {
        submitPreview();
    }
}

void PipelineController::setBlackPct(float value) {
    if (m_params.levels.blackPct == value) {
        return;
    }
    m_params.levels.blackPct = value;
    emit paramsChanged();
    submitPreview();
}

void PipelineController::setWhitePct(float value) {
    if (m_params.levels.whitePct == value) {
        return;
    }
    m_params.levels.whitePct = value;
    emit paramsChanged();
    submitPreview();
}

void PipelineController::setClaheClip(float value) {
    if (m_params.clahe.clipLimit == value) {
        return;
    }
    m_params.clahe.clipLimit = value;
    emit paramsChanged();
    submitPreview();
}

void PipelineController::setClaheTile(int value) {
    if (m_params.clahe.tilesX == value && m_params.clahe.tilesY == value) {
        return;
    }
    m_params.clahe.tilesX = value;
    m_params.clahe.tilesY = value;
    emit paramsChanged();
    submitPreview();
}

void PipelineController::setBusy(bool value) {
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void PipelineController::submitPreview() {
    if (!m_document || !m_document->buffer()) {
        return;
    }

    // Shallow copies (ImageBuffer's HostStorage::data is a shared_ptr;
    // Params is plain value data) so the job closure below owns what it
    // needs independent of m_document/m_params still changing on the UI
    // thread while this runs on a worker (PLAN.md §6.1: a published
    // ImageBuffer's pixels are never mutated in place, so sharing the
    // same underlying buffer across threads is safe).
    auto sourceCopy = std::make_shared<luma::ImageBuffer>(*m_document->buffer());
    const luma::Params params = m_params;
    ImageDocument* const requestDoc = m_document.data();
    const auto docId = static_cast<luma::DocumentId>(reinterpret_cast<std::uintptr_t>(requestDoc));
    const std::uint64_t seq = ++m_requestSeq;

    // Both slots are local to this one job's pair of closures, not
    // PipelineController members - JobScheduler guarantees PREVIEW jobs
    // for one document never run concurrently with each other (a single
    // dedicated preview worker, JobScheduler.cpp), so there's no race on
    // these between submission and onComplete.
    auto resultSlot = std::make_shared<std::shared_ptr<const luma::PipelineResult>>();
    auto errorSlot = std::make_shared<QString>();

    setBusy(true);
    m_scheduler.submit(
        docId, luma::JobKind::Preview,
        [sourceCopy, params, resultSlot, errorSlot](const luma::CancelToken& token) {
            const luma::ImageBuffer proxy = luma::buildPreviewProxy(*sourceCopy);
            if (token.cancelled()) {
                return;
            }
            auto result = luma::runPipeline(proxy, params);
            if (token.cancelled()) {
                return;
            }
            if (result) {
                *resultSlot =
                    std::make_shared<const luma::PipelineResult>(std::move(result.value()));
            } else {
                *errorSlot = QStringLiteral("Pipeline failed on this image's colour space");
            }
        },
        [this, requestDoc, seq, resultSlot, errorSlot]() {
            // onComplete runs on the worker thread that just finished
            // the job above; invokeMethod with `this` as the context
            // object hands the rest off to the UI thread and is a no-op
            // if `this` is destroyed by the time it's processed.
            QMetaObject::invokeMethod(
                this,
                [this, requestDoc, seq, resultSlot, errorSlot]() {
                    // Only the most-recently-*submitted* request's own
                    // completion ever clears busy. A naive +1/-1 pending
                    // count can't do this safely: JobScheduler silently
                    // drops a PREVIEW job that gets superseded while
                    // still queued (JobScheduler.cpp - no completion
                    // callback at all, not even a "cancelled" one), so
                    // an older request's "+1" would never see a matching
                    // "-1" and busy() would get stuck true. The *latest*
                    // request, by definition, is never superseded by
                    // anything after it, so it's always guaranteed to
                    // eventually complete and reach here - nothing else
                    // needs to clear busy (CodeRabbit review on PR #20).
                    if (seq == m_requestSeq) {
                        setBusy(false);
                    }
                    // Separately: JobScheduler's latest-only slot only
                    // dedups PREVIEW jobs for the *same* DocumentId; it
                    // has no notion of "the controller switched to a
                    // different document" (a switch starts an
                    // independent job under a different DocumentId, so
                    // it isn't superseded by anything and completes
                    // normally). requestDoc is a plain pointer captured
                    // at submission time purely for this identity check
                    // (never dereferenced) - if m_document has since
                    // changed, this result is for a document we're no
                    // longer showing, so it's dropped rather than
                    // delivered as if current (CodeRabbit review on PR #20).
                    if (requestDoc != m_document) {
                        return;
                    }
                    if (*resultSlot) {
                        emit previewReady(*resultSlot);
                    } else if (!errorSlot->isEmpty()) {
                        emit previewFailed(*errorSlot);
                    }
                },
                Qt::QueuedConnection);
        });
}

} // namespace luma::ui
