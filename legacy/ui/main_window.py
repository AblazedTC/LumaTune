from __future__ import annotations

import os

import cv2
import numpy as np
from PySide6.QtCore import QObject, QRunnable, Qt, QThreadPool, QTimer, Signal
from PySide6.QtGui import QAction, QImage, QPixmap
from PySide6.QtWidgets import (
    QFileDialog,
    QGroupBox,
    QLabel,
    QMainWindow,
    QSizePolicy,
    QSlider,
    QSplitter,
    QStatusBar,
    QTextEdit,
    QToolBar,
    QVBoxLayout,
    QWidget,
)

from models.image_state import ImageState
from processing.pipeline import Pipeline, ProcessingParams
from ui.histogram_widget import HistogramWidget


def ndarray_to_pixmap(rgb_float: np.ndarray) -> QPixmap:
    """Convert an HxWx3 float32 [0, 1] RGB array to a QPixmap."""
    rgb_u8 = np.ascontiguousarray((rgb_float * 255.0).clip(0, 255).astype(np.uint8))
    h, w, ch = rgb_u8.shape
    img = QImage(rgb_u8.data, w, h, w * ch, QImage.Format.Format_RGB888)
    # .copy() detaches QImage from the numpy buffer before it can be freed
    return QPixmap.fromImage(img.copy())


class _WorkerSignals(QObject):
    result_ready = Signal(object, object)
    error = Signal(str)


class _PipelineWorker(QRunnable):
    def __init__(self, state: ImageState, params: ProcessingParams) -> None:
        super().__init__()
        self._state = state
        self._params = params
        self.signals = _WorkerSignals()

    def run(self) -> None:
        try:
            output, diag = Pipeline.run(self._state, self._params)
            self.signals.result_ready.emit(output, diag)
        except Exception as exc:
            self.signals.error.emit(str(exc))


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Image Enhancement Pipeline — V1")
        self.resize(1200, 800)

        self._state: ImageState | None = None
        self._last_output: np.ndarray | None = None

        self._debounce = QTimer(self)
        self._debounce.setSingleShot(True)
        self._debounce.timeout.connect(self._run_pipeline)

        self._build_toolbar()
        self._build_ui()
        self.setStatusBar(QStatusBar())

    # ------------------------------------------------------------------
    # Construction
    # ------------------------------------------------------------------

    def _build_toolbar(self) -> None:
        tb = QToolBar("Main", self)
        tb.setMovable(False)
        self.addToolBar(tb)

        open_act = QAction("Open Image…", self)
        open_act.triggered.connect(self._open_image)
        tb.addAction(open_act)

        enhance_act = QAction("Auto Enhance", self)
        enhance_act.triggered.connect(self._run_pipeline)
        tb.addAction(enhance_act)

        export_act = QAction("Export…", self)
        export_act.triggered.connect(self._export_image)
        tb.addAction(export_act)

    def _build_ui(self) -> None:
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(4, 4, 4, 4)
        root.setSpacing(4)

        # Top row: images side-by-side + control panel
        top = QSplitter(Qt.Orientation.Horizontal)

        imgs = QSplitter(Qt.Orientation.Horizontal)
        self._before_label = self._make_image_label("Before")
        self._after_label = self._make_image_label("After")
        imgs.addWidget(self._before_label)
        imgs.addWidget(self._after_label)
        imgs.setSizes([500, 500])

        top.addWidget(imgs)
        top.addWidget(self._build_control_panel())
        top.setSizes([920, 260])

        root.addWidget(top, stretch=3)

        # Bottom: histogram
        self._histogram = HistogramWidget()
        self._histogram.setMinimumHeight(90)
        self._histogram.setMaximumHeight(120)
        root.addWidget(self._histogram, stretch=0)

    def _make_image_label(self, placeholder: str) -> QLabel:
        lbl = QLabel(placeholder)
        lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        lbl.setStyleSheet("background:#1e1e1e; color:#555; font-size:14px;")
        return lbl

    def _build_control_panel(self) -> QWidget:
        panel = QWidget()
        panel.setFixedWidth(260)
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(6, 6, 6, 6)

        sh = QGroupBox("Shadows / Highlights")
        sh_l = QVBoxLayout(sh)
        # int 0–50 → black_pct 0.0–5.0; default 5 → 0.5
        self._shadow_slider = self._add_slider("Shadows", 0, 50, 5, sh_l)
        # int 0–50 → white_pct 95.0–100.0; default 45 → 99.5
        self._highlight_slider = self._add_slider("Highlights", 0, 50, 45, sh_l)
        layout.addWidget(sh)

        lc = QGroupBox("Local Contrast")
        lc_l = QVBoxLayout(lc)
        # int 10–100 → clahe_clip 1.0–10.0; default 20 → 2.0
        self._clarity_slider = self._add_slider("Clarity", 10, 100, 20, lc_l)
        # int 4–16 → tile size directly; default 8
        self._tile_slider = self._add_slider("Tile Size", 4, 16, 8, lc_l)
        layout.addWidget(lc)

        diag = QGroupBox("Diagnostics")
        diag_l = QVBoxLayout(diag)
        self._diag_text = QTextEdit()
        self._diag_text.setReadOnly(True)
        self._diag_text.setStyleSheet("font-family:monospace; font-size:11px;")
        self._diag_text.setMinimumHeight(160)
        diag_l.addWidget(self._diag_text)
        layout.addWidget(diag)

        layout.addStretch()
        return panel

    def _add_slider(
        self, label: str, lo: int, hi: int, default: int, parent_layout: QVBoxLayout
    ) -> QSlider:
        parent_layout.addWidget(QLabel(label))
        slider = QSlider(Qt.Orientation.Horizontal)
        slider.setRange(lo, hi)
        slider.setValue(default)
        slider.valueChanged.connect(lambda _: self._debounce.start(200))
        parent_layout.addWidget(slider)
        return slider

    # ------------------------------------------------------------------
    # Slider → ProcessingParams
    # ------------------------------------------------------------------

    def _current_params(self) -> ProcessingParams:
        return ProcessingParams(
            black_pct=self._shadow_slider.value() / 10.0,
            white_pct=95.0 + self._highlight_slider.value() / 10.0,
            clahe_clip=self._clarity_slider.value() / 10.0,
            clahe_tile=self._tile_slider.value(),
        )

    # ------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------

    def _open_image(self) -> None:
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Image",
            "",
            "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.webp)",
        )
        if not path:
            return
        try:
            self._state = ImageState.load(path)
            self._set_pixmap(self._before_label, ndarray_to_pixmap(self._state.original_rgb))
            self._after_label.setPixmap(QPixmap())
            self._after_label.setText("After")
            self._last_output = None
            self.statusBar().showMessage(f"Loaded: {os.path.basename(path)}")
            self._run_pipeline()
        except Exception as exc:
            self.statusBar().showMessage(f"Error loading image: {exc}")

    def _run_pipeline(self) -> None:
        if self._state is None or self._state.original_rgb is None:
            return
        params = self._current_params()
        worker = _PipelineWorker(self._state, params)
        worker.signals.result_ready.connect(self._on_result_ready)
        worker.signals.error.connect(
            lambda msg: self.statusBar().showMessage(f"Pipeline error: {msg}")
        )
        QThreadPool.globalInstance().start(worker)

    def _export_image(self) -> None:
        if self._last_output is None:
            self.statusBar().showMessage("Nothing to export — open and enhance an image first.")
            return
        path, _ = QFileDialog.getSaveFileName(
            self, "Export Image", "enhanced.png", "PNG (*.png);;JPEG (*.jpg *.jpeg)"
        )
        if not path:
            return
        out_u8 = (self._last_output * 255.0).clip(0, 255).astype(np.uint8)
        out_bgr = cv2.cvtColor(out_u8, cv2.COLOR_RGB2BGR)
        cv2.imwrite(path, out_bgr)
        self.statusBar().showMessage(f"Exported: {os.path.basename(path)}")

    # ------------------------------------------------------------------
    # Result
    # ------------------------------------------------------------------

    def _on_result_ready(self, output: np.ndarray, diag: dict) -> None:
        self._last_output = output
        self._set_pixmap(self._after_label, ndarray_to_pixmap(output))
        self._histogram.set_data(
            diag["hist_before"],
            diag["hist_after"],
            diag["levels"]["black_point"],
            diag["levels"]["white_point"],
        )
        self._diag_text.setPlainText(_format_diagnostics(diag))

    def _set_pixmap(self, label: QLabel, px: QPixmap) -> None:
        label.setPixmap(
            px.scaled(label.size(), Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
        )

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        if self._state is not None:
            self._set_pixmap(self._before_label, ndarray_to_pixmap(self._state.original_rgb))
        if self._last_output is not None:
            self._set_pixmap(self._after_label, ndarray_to_pixmap(self._last_output))


def _format_diagnostics(diag: dict) -> str:
    wb = diag["wb"]
    lv = diag["levels"]
    cl = diag["clahe"]
    return (
        f"White Balance (Gray World)\n"
        f"  avg  R:{wb['avg_r']:6.1f}  G:{wb['avg_g']:6.1f}  B:{wb['avg_b']:6.1f}\n"
        f"  mult R:\u00d7{wb['mult_r']:.3f}  G:\u00d7{wb['mult_g']:.3f}  B:\u00d7{wb['mult_b']:.3f}\n"
        f"\n"
        f"Auto Levels\n"
        f"  Input range:  {lv['original_min']:.0f} \u2192 {lv['original_max']:.0f}\n"
        f"  Black point:  {lv['black_point']:.1f}  ({lv['black_pct']:.2f} pct)\n"
        f"  White point:  {lv['white_point']:.1f}  ({lv['white_pct']:.2f} pct)\n"
        f"\n"
        f"CLAHE\n"
        f"  Clip limit:   {cl['clip_limit']:.1f}\n"
        f"  Tile size:    {cl['tile_size']} \u00d7 {cl['tile_size']}"
    )
