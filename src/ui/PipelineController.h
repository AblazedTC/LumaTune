#pragma once

#include <cstdint>
#include <memory>

#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>

#include "../core/engine/JobScheduler.h"
#include "../core/engine/Params.h"
#include "../core/engine/Pipeline.h"
#include "ImageDocument.h"

namespace luma::ui {

// Owns the JobScheduler (PLAN.md §7.6/§8.2) and (re)submits a PREVIEW job
// whenever its document or params change. JobScheduler's own
// latest-only-slot + generation-matched-delivery logic (JobScheduler.h)
// already drops stale results, so this class doesn't duplicate any of
// that bookkeeping itself - it only has to shepherd each surviving job's
// result back onto the UI thread, via QMetaObject::invokeMethod(this, ...,
// Qt::QueuedConnection) (PLAN.md §8.2's "Results cross threads via
// Qt::QueuedConnection"), which Qt safely no-ops if `this` was destroyed
// in the meantime (the documented behaviour of that overload's "context
// object" argument).
//
// Only the four V1-parity params (PLAN.md P2.3: "V1's four sliders") get
// a Q_PROPERTY so far; the rest of luma::Params (WB method/strength,
// curve knots, saturation) gets its own as Controls.qml grows into them
// in P2.3 - adding unused properties now would be speculative.
//
// This is P2.2's controller half only. PreviewItem's RGBA16F QSGTexture
// upload is a separate, later chunk - previewReady() just hands out the
// raw luma::PipelineResult; nothing paints it yet, and nothing in this
// class is wired into Main.qml until that chunk lands.
class PipelineController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(luma::ui::ImageDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(float blackPct READ blackPct WRITE setBlackPct NOTIFY paramsChanged)
    Q_PROPERTY(float whitePct READ whitePct WRITE setWhitePct NOTIFY paramsChanged)
    Q_PROPERTY(float claheClip READ claheClip WRITE setClaheClip NOTIFY paramsChanged)
    Q_PROPERTY(int claheTile READ claheTile WRITE setClaheTile NOTIFY paramsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit PipelineController(QObject* parent = nullptr);
    ~PipelineController() override;

    ImageDocument* document() const { return m_document; }
    void setDocument(ImageDocument* document);

    float blackPct() const { return m_params.levels.blackPct; }
    void setBlackPct(float value);
    float whitePct() const { return m_params.levels.whitePct; }
    void setWhitePct(float value);
    float claheClip() const { return m_params.clahe.clipLimit; }
    void setClaheClip(float value);
    int claheTile() const { return m_params.clahe.tilesX; }
    void setClaheTile(int value);

    bool busy() const { return m_busy; }

signals:
    void documentChanged();
    void paramsChanged();
    void busyChanged();
    // Not QML-representable (same reasoning as ImageDocument::buffer()) -
    // only a C++ listener (PreviewItem, once it exists; QSignalSpy today)
    // can connect to this.
    void previewReady(std::shared_ptr<const luma::PipelineResult> result);
    void previewFailed(QString message);

private:
    void submitPreview();
    void setBusy(bool value);

    luma::JobScheduler m_scheduler;
    // QPointer, not a raw pointer: ImageDocument's lifetime isn't owned
    // by this class (QML/whoever calls setDocument() owns it), so it can
    // be destroyed out from under us - QPointer auto-clears to nullptr
    // when that happens instead of leaving a dangling pointer for the
    // next setBlackPct()/etc. to dereference via submitPreview()
    // (CodeRabbit review on PR #20).
    QPointer<ImageDocument> m_document;
    QMetaObject::Connection m_documentConnection;
    luma::Params m_params;
    // Bumped on every submitPreview() call; a completion only clears
    // busy() if its own captured value still equals this (PipelineController.cpp:
    // "only the latest submission's own completion clears busy"). Needed
    // because JobScheduler silently drops a job that's superseded while
    // still queued - no completion callback at all, not even a
    // "cancelled" one - so a naive pending-job *count* would leak an
    // unmatched increment for every such job and get stuck busy (a real
    // bug CodeRabbit's review caught in this class's first attempt at
    // this: tracking "is the most recent request still outstanding" by
    // identity, as done here, needs no matching per-job decrement to
    // begin with).
    std::uint64_t m_requestSeq{0};
    bool m_busy{false};
};

} // namespace luma::ui

// Required for this to cross a signal/slot connection at all - QSignalSpy
// (and any QueuedConnection) copies signal arguments into QVariant, which
// needs the type registered as a QMetaType; shared_ptr is default-
// constructible/copyable/destructible so it qualifies. Actually calling
// qRegisterMetaType<T>() still happens in PipelineController's constructor
// (Q_DECLARE_METATYPE alone registers the type statically but a runtime
// qRegisterMetaType call is what's needed before it's first used in a
// connection, e.g. by QSignalSpy in tests/ui/PipelineControllerTest.cpp).
Q_DECLARE_METATYPE(std::shared_ptr<const luma::PipelineResult>)
