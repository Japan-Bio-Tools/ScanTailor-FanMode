// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <core/TiffWriter.h>
#include <filters/ocr/ImagePdfWriter.h>

#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <string>

BOOST_AUTO_TEST_SUITE(ImagePdfWriterTestSuite)

BOOST_AUTO_TEST_CASE(creates_multi_page_pdf_without_a_text_layer) {
  int argc = 1;
  char applicationName[] = "image-pdf-writer-test";
  char* argv[] = {applicationName, nullptr};
  QCoreApplication application(argc, argv);

  QTemporaryDir directory;
  BOOST_REQUIRE(directory.isValid());

  QImage monochrome(300, 600, QImage::Format_Mono);
  monochrome.fill(1);
  monochrome.setDotsPerMeterX(qRound(300.0 / 0.0254));
  monochrome.setDotsPerMeterY(qRound(300.0 / 0.0254));
  const QString firstImage = QDir(directory.path()).filePath(QStringLiteral("page-01.png"));
  BOOST_REQUIRE(monochrome.save(firstImage));

  QImage color(400, 200, QImage::Format_RGB32);
  color.fill(QColor(40, 100, 180));
  color.setDotsPerMeterX(qRound(200.0 / 0.0254));
  color.setDotsPerMeterY(qRound(200.0 / 0.0254));
  const QString secondImage = QDir(directory.path()).filePath(QStringLiteral("page-02.png"));
  BOOST_REQUIRE(color.save(secondImage));

  const QString pdfPath = QDir(directory.path()).filePath(QStringLiteral("book.pdf"));
  QString error;
  int lastProgress = -1;
  const bool written = ocr::ImagePdfWriter::write(
      {firstImage, secondImage}, pdfPath, 82,
      [&](const int completed, const int) {
        lastProgress = completed;
        return true;
      },
      &error);
  BOOST_REQUIRE_MESSAGE(written, error.toStdString());
  BOOST_CHECK_EQUAL(lastProgress, 2);
  BOOST_CHECK(QFileInfo(pdfPath).size() > 0);

  QPDF pdf;
  pdf.processFile(pdfPath.toUtf8().constData());
  std::vector<QPDFPageObjectHelper> pages = QPDFPageDocumentHelper(pdf).getAllPages();
  BOOST_REQUIRE_EQUAL(pages.size(), 2);

  const QPDFObjectHandle firstBox = pages.at(0).getMediaBox();
  BOOST_CHECK_SMALL(firstBox.getArrayItem(2).getNumericValue() - 72.0, 0.05);
  BOOST_CHECK_SMALL(firstBox.getArrayItem(3).getNumericValue() - 144.0, 0.05);
  const QPDFObjectHandle secondBox = pages.at(1).getMediaBox();
  BOOST_CHECK_SMALL(secondBox.getArrayItem(2).getNumericValue() - 144.0, 0.05);
  BOOST_CHECK_SMALL(secondBox.getArrayItem(3).getNumericValue() - 72.0, 0.05);

  for (QPDFPageObjectHelper& page : pages) {
    std::string contents;
    for (QPDFObjectHandle stream : page.getPageContents()) {
      const std::shared_ptr<Buffer> data = stream.getStreamData(qpdf_dl_generalized);
      contents.append(reinterpret_cast<const char*>(data->getBuffer()), data->getSize());
    }
    BOOST_CHECK(contents.find("/Im0 Do") != std::string::npos);
    BOOST_CHECK(contents.find("BT") == std::string::npos);
  }
}

BOOST_AUTO_TEST_CASE(cancel_before_first_page_writes_nothing) {
  QTemporaryDir directory;
  BOOST_REQUIRE(directory.isValid());
  QImage image(20, 20, QImage::Format_RGB32);
  image.fill(Qt::white);
  const QString imagePath = QDir(directory.path()).filePath(QStringLiteral("page.png"));
  const QString pdfPath = QDir(directory.path()).filePath(QStringLiteral("canceled.pdf"));
  BOOST_REQUIRE(image.save(imagePath));

  QString error;
  BOOST_CHECK(
      !ocr::ImagePdfWriter::write({imagePath}, pdfPath, 82, [](const int, const int) { return false; }, &error));
  BOOST_CHECK(error.isEmpty());
  BOOST_CHECK(!QFileInfo::exists(pdfPath));
}

BOOST_AUTO_TEST_CASE(reads_scan_tailor_output_tiff_without_a_qt_tiff_plugin) {
  QTemporaryDir directory;
  BOOST_REQUIRE(directory.isValid());

  QImage page(640, 900, QImage::Format_RGB32);
  page.fill(QColor(248, 246, 238));
  for (int y = 100; y < 800; y += 45) {
    for (int x = 80; x < 560; ++x) {
      page.setPixelColor(x, y, Qt::black);
    }
  }
  page.setDotsPerMeterX(qRound(300.0 / 0.0254));
  page.setDotsPerMeterY(qRound(300.0 / 0.0254));

  const QString tiffPath = QDir(directory.path()).filePath(QStringLiteral("page-001.tif"));
  BOOST_REQUIRE(TiffWriter::writeImage(tiffPath, page));

  const QString pdfPath = QDir(directory.path()).filePath(QStringLiteral("image-only.pdf"));
  QString error;
  BOOST_REQUIRE_MESSAGE(ocr::ImagePdfWriter::write({tiffPath}, pdfPath, 85, {}, &error), error.toStdString());
  BOOST_CHECK(QFileInfo(pdfPath).size() > 0);

  QPDF pdf;
  pdf.processFile(pdfPath.toUtf8().constData());
  std::vector<QPDFPageObjectHelper> pages = QPDFPageDocumentHelper(pdf).getAllPages();
  BOOST_REQUIRE_EQUAL(pages.size(), 1);
  const QPDFObjectHandle mediaBox = pages.front().getMediaBox();
  BOOST_CHECK_SMALL(mediaBox.getArrayItem(2).getNumericValue() - 153.6, 0.05);
  BOOST_CHECK_SMALL(mediaBox.getArrayItem(3).getNumericValue() - 216.0, 0.05);
}

BOOST_AUTO_TEST_SUITE_END()
