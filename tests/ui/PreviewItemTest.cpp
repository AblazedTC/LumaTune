#include <QFloat16>
#include <QObject>
#include <QTest>

#include "PreviewItem.h"
#include "image/BufferPool.h"

// T-6 (PLAN.md §11). Only toRgba16FImage() is covered here - it's the
// part of PreviewItem that doesn't need a QQuickWindow/GPU context.
// updatePaintNode()'s actual QSGTexture upload is exercised by launching
// the app (AGENTS.md §4) instead, same as src/ui/** changes generally.
class PreviewItemTest : public QObject {
    Q_OBJECT
private slots:
    void convertsRgbF32ToRgba16F();
};

void PreviewItemTest::convertsRgbF32ToRgba16F() {
    luma::ImageBuffer buffer =
        luma::BufferPool::instance().acquire(2, 1, luma::PixelFormat::RGB_F32, luma::ColorSpace{});
    cv::Mat mat = buffer.mat();
    mat.at<cv::Vec3f>(0, 0) = cv::Vec3f(1.0f, 0.5f, 0.0f);
    mat.at<cv::Vec3f>(0, 1) = cv::Vec3f(0.25f, 0.75f, 1.0f);

    const QImage image = luma::ui::toRgba16FImage(buffer);
    QCOMPARE(image.width(), 2);
    QCOMPARE(image.height(), 1);
    QCOMPARE(image.format(), QImage::Format_RGBA16FPx4);

    const auto* row = reinterpret_cast<const qfloat16*>(image.constScanLine(0));
    QCOMPARE(float(row[0]), 1.0f);
    QCOMPARE(float(row[1]), 0.5f);
    QCOMPARE(float(row[2]), 0.0f);
    QCOMPARE(float(row[3]), 1.0f); // alpha always opaque
    QCOMPARE(float(row[4]), 0.25f);
    QCOMPARE(float(row[5]), 0.75f);
    QCOMPARE(float(row[6]), 1.0f);
    QCOMPARE(float(row[7]), 1.0f);

    luma::BufferPool::instance().release(std::move(buffer));
}

QTEST_GUILESS_MAIN(PreviewItemTest)
#include "PreviewItemTest.moc"
