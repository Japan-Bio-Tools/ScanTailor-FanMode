// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/ocr/RtlPdfFixer.h>

#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(RtlPdfFixerTestSuite)

BOOST_AUTO_TEST_CASE(replaces_mirrored_text_with_bidi_safe_text) {
  QTemporaryDir directory;
  BOOST_REQUIRE(directory.isValid());
  const QString inputPath = QDir(directory.path()).filePath("input.pdf");
  const QString tsvPath = QDir(directory.path()).filePath("input.tsv");
  const QString outputPath = QDir(directory.path()).filePath("output.pdf");

  {
    QPDF pdf;
    pdf.emptyPDF();
    const QPDFObjectHandle font = pdf.makeIndirectObject(
        QPDFObjectHandle::parse(&pdf, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"));
    const QPDFObjectHandle contents = pdf.newStream(
        "q\n0 0 m\nQ\n"
        "BT\n3 Tr -1 0 0 1 90 100 Tm /f-0-0 12 Tf [ <06450631062d062806270020> ] TJ\nET\n");
    const QPDFObjectHandle page = QPDFObjectHandle::parse(
        &pdf, "<< /Type /Page /MediaBox [0 0 100 200] /Resources << /Font << /f-0-0 "
                  + font.unparse() + " >> >> /Contents " + contents.unparse() + " >>");
    pdf.addPage(page, false);
    QPDFWriter writer(pdf, inputPath.toUtf8().constData());
    writer.write();
  }

  QFile tsv(tsvPath);
  BOOST_REQUIRE(tsv.open(QIODevice::WriteOnly));
  const QByteArray data = QByteArrayLiteral(
      "level\tpage_num\tblock_num\tpar_num\tline_num\tword_num\tleft\ttop\twidth\theight\tconf\ttext\n"
      "1\t1\t0\t0\t0\t0\t0\t0\t1000\t2000\t-1\t\n"
      "5\t1\t1\t1\t1\t1\t600\t800\t300\t100\t95\t\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7\n"
      "5\t1\t1\t1\t1\t2\t100\t800\t300\t100\t95\tWindows\xE2\x80\x8F\n");
  BOOST_REQUIRE_EQUAL(tsv.write(data), data.size());
  tsv.close();

  QString error;
  BOOST_REQUIRE_MESSAGE(ocr::RtlPdfFixer::rewrite(inputPath, tsvPath, outputPath, &error), error.toStdString());

  QPDF fixed;
  fixed.processFile(outputPath.toUtf8().constData());
  QPDFPageDocumentHelper document(fixed);
  std::vector<QPDFPageObjectHelper> pages = document.getAllPages();
  BOOST_REQUIRE_EQUAL(pages.size(), 1);
  std::string combined;
  for (QPDFObjectHandle content : pages.front().getPageContents()) {
    const std::shared_ptr<Buffer> buffer = content.getStreamData(qpdf_dl_generalized);
    combined.append(reinterpret_cast<const char*>(buffer->getBuffer()), buffer->getSize());
  }

  BOOST_CHECK(combined.find("0 0 m") != std::string::npos);
  BOOST_CHECK(combined.find("-1 0 0 1") == std::string::npos);
  BOOST_CHECK(combined.find("1 0 0 1") != std::string::npos);
  // RTL code points, including the separator, are stored in visual order.  A
  // reader's bidi pass then returns the original logical word "مرحبا".
  BOOST_CHECK(combined.find("<002006270628062d06310645>") != std::string::npos);
  // A trailing RTL mark must not reverse a word whose first strong character is Latin.
  BOOST_CHECK(combined.find("<00570069006e0064006f00770073200f0020>") != std::string::npos);
  BOOST_CHECK(combined.find("/ActualText") == std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
