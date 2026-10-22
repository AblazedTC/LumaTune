from __future__ import annotations

import numpy as np
from PySide6.QtCore import Qt, QPointF
from PySide6.QtGui import QColor, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget


class HistogramWidget(QWidget):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._before: np.ndarray | None = None
        self._after: np.ndarray | None = None
        self._black_pt: float = 0.0
        self._white_pt: float = 255.0
        self.setMinimumHeight(80)

    def set_data(
        self,
        before: np.ndarray,
        after: np.ndarray,
        black_pt: float,
        white_pt: float,
    ) -> None:
        self._before = before
        self._after = after
        self._black_pt = black_pt
        self._white_pt = white_pt
        self.update()

    def paintEvent(self, event) -> None:
        if self._before is None:
            return
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        w, h = self.width(), self.height()

        painter.fillRect(0, 0, w, h, QColor(30, 30, 30))

        self._draw_hist(painter, self._before, QColor(110, 110, 110, 170), w, h)
        self._draw_hist(painter, self._after, QColor(220, 220, 220, 200), w, h)

        # Black point (blue) and white point (orange) markers
        bx = int(self._black_pt / 255.0 * w)
        wx = int(self._white_pt / 255.0 * w)
        painter.setPen(QPen(QColor(80, 140, 255), 1))
        painter.drawLine(bx, 0, bx, h)
        painter.setPen(QPen(QColor(255, 160, 40), 1))
        painter.drawLine(wx, 0, wx, h)

    def _draw_hist(
        self, painter: QPainter, hist: np.ndarray, color: QColor, w: int, h: int
    ) -> None:
        peak = float(hist.max()) if hist.max() > 0 else 1.0
        n = len(hist)
        poly = QPolygonF()
        poly.append(QPointF(0.0, float(h)))
        for i, v in enumerate(hist):
            x = i / (n - 1) * w
            y = h - (v / peak) * (h - 4)
            poly.append(QPointF(x, y))
        poly.append(QPointF(float(w), float(h)))

        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(color)
        painter.drawPolygon(poly)
