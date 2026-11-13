#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QUrl>

#include "ImageDocument.h"

namespace {
QString goldenFixture(const QString& name) {
    return QString(LUMA_REPO_ROOT) + "/tests/golden/fixtures/" + name;
}
} // namespace

// T-6 (PLAN.md §11): controllers are tested with QTest/QSignalSpy.
// ImageDocument decodes synchronously (AGENTS.md §10/ImageDocument.h), so
// these are all plain synchronous calls - no event loop needed.
class ImageDocumentTest : public QObject {
    Q_OBJECT
private slots:
    void opensValidFixture();
    void reportsErrorForMissingFile();
    void acceptsFileUrl();
    void rejectsNonLocalUrl();
    void clearsErrorStringOnSuccessfulReopen();
};

void ImageDocumentTest::opensValidFixture() {
    luma::ui::ImageDocument doc;
    const QString path = goldenFixture("neutral.png");
    QVERIFY(doc.openPath(path));
    QVERIFY(doc.loaded());
    QCOMPARE(doc.path(), path);
    QVERIFY(doc.errorString().isEmpty());
    QVERIFY(doc.buffer() != nullptr);
    QCOMPARE(doc.buffer()->width, 480);
    QCOMPARE(doc.buffer()->height, 360);
}

void ImageDocumentTest::reportsErrorForMissingFile() {
    luma::ui::ImageDocument doc;
    QSignalSpy spy(&doc, &luma::ui::ImageDocument::errorOccurred);
    QVERIFY(!doc.openPath(QStringLiteral("/nonexistent/path/does-not-exist.png")));
    QVERIFY(!doc.loaded());
    QVERIFY(doc.buffer() == nullptr);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!doc.errorString().isEmpty());
}

void ImageDocumentTest::acceptsFileUrl() {
    luma::ui::ImageDocument doc;
    QVERIFY(doc.open(QUrl::fromLocalFile(goldenFixture("neutral.png"))));
    QVERIFY(doc.loaded());
}

void ImageDocumentTest::rejectsNonLocalUrl() {
    luma::ui::ImageDocument doc;
    QSignalSpy spy(&doc, &luma::ui::ImageDocument::errorOccurred);
    QVERIFY(!doc.open(QUrl(QStringLiteral("https://example.com/image.png"))));
    QVERIFY(!doc.loaded());
    QCOMPARE(spy.count(), 1);
}

void ImageDocumentTest::clearsErrorStringOnSuccessfulReopen() {
    // CodeRabbit review on PR #20: errorString's own NOTIFY must fire on
    // the clearing path too, or a QML binding to it would keep showing a
    // stale error after a later successful open.
    luma::ui::ImageDocument doc;
    QVERIFY(!doc.openPath(QStringLiteral("/nonexistent/path/does-not-exist.png")));
    QVERIFY(!doc.errorString().isEmpty());

    QSignalSpy spy(&doc, &luma::ui::ImageDocument::errorStringChanged);
    QVERIFY(doc.openPath(goldenFixture("neutral.png")));
    QVERIFY(doc.errorString().isEmpty());
    QCOMPARE(spy.count(), 1);
}

QTEST_GUILESS_MAIN(ImageDocumentTest)
#include "ImageDocumentTest.moc"
