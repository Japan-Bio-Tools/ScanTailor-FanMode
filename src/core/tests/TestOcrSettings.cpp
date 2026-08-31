// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/ocr/Settings.h>

#include <QDomDocument>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(OcrSettingsTestSuite)

BOOST_AUTO_TEST_CASE(defaults_are_bilingual_and_balanced) {
  const ocr::Settings settings;

  BOOST_CHECK(settings.arabicEnabled());
  BOOST_CHECK(settings.englishEnabled());
  BOOST_CHECK(settings.compression() == ocr::PdfCompression::BALANCED);
  BOOST_CHECK_EQUAL(settings.jpegQuality(), 82);
  BOOST_CHECK_CLOSE(settings.estimatedSizeRatio(), 0.08, 0.001);
}

BOOST_AUTO_TEST_CASE(compression_presets_have_expected_quality_order) {
  ocr::Settings settings;

  settings.setCompression(ocr::PdfCompression::HIGH_QUALITY);
  const int highQuality = settings.jpegQuality();
  const double highRatio = settings.estimatedSizeRatio();
  settings.setCompression(ocr::PdfCompression::BALANCED);
  const int balancedQuality = settings.jpegQuality();
  const double balancedRatio = settings.estimatedSizeRatio();
  settings.setCompression(ocr::PdfCompression::SMALL);

  BOOST_CHECK_GT(highQuality, balancedQuality);
  BOOST_CHECK_GT(balancedQuality, settings.jpegQuality());
  BOOST_CHECK_GT(highRatio, balancedRatio);
  BOOST_CHECK_GT(balancedRatio, settings.estimatedSizeRatio());
}

BOOST_AUTO_TEST_CASE(project_xml_round_trip_preserves_selection) {
  ocr::Settings settings;
  settings.setArabicEnabled(false);
  settings.setEnglishEnabled(true);
  settings.setCompression(ocr::PdfCompression::SMALL);

  QDomDocument document;
  const QDomElement element = settings.toXml(document, QStringLiteral("ocr"));
  const ocr::Settings restored(element);

  BOOST_CHECK(!restored.arabicEnabled());
  BOOST_CHECK(restored.englishEnabled());
  BOOST_CHECK(restored.compression() == ocr::PdfCompression::SMALL);
  BOOST_CHECK_EQUAL(restored.jpegQuality(), 60);
}

BOOST_AUTO_TEST_CASE(unknown_compression_falls_back_to_balanced) {
  QDomDocument document;
  QDomElement element = document.createElement(QStringLiteral("ocr"));
  element.setAttribute(QStringLiteral("compression"), 99);

  const ocr::Settings restored(element);
  BOOST_CHECK(restored.compression() == ocr::PdfCompression::BALANCED);
}

BOOST_AUTO_TEST_SUITE_END()
