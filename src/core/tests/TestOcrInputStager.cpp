// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/ocr/InputFileStager.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(OcrInputStagerTestSuite)

BOOST_AUTO_TEST_CASE(stages_unicode_source_under_ascii_filename) {
  QTemporaryDir temporary;
  BOOST_REQUIRE(temporary.isValid());
  const QString unicodeDirectory
      = QDir(temporary.path()).filePath(QString::fromUtf8("1 Ra\xC7\xA7" "ab 1365"));
  BOOST_REQUIRE(QDir().mkpath(unicodeDirectory));
  const QString sourcePath = QDir(unicodeDirectory).filePath(QStringLiteral("page-01.tif"));
  QFile source(sourcePath);
  BOOST_REQUIRE(source.open(QIODevice::WriteOnly));
  BOOST_REQUIRE_EQUAL(source.write("TIFF test page"), 14);
  source.close();

  QStringList stagedFiles;
  QString error;
  const QString destination = QDir(temporary.path()).filePath(QStringLiteral("ocr-input"));
  BOOST_REQUIRE_MESSAGE(ocr::InputFileStager::stage({sourcePath}, destination, &stagedFiles, &error),
                        error.toStdString());
  BOOST_REQUIRE_EQUAL(stagedFiles.size(), 1);
  BOOST_CHECK_EQUAL(QFileInfo(stagedFiles.front()).fileName().toStdString(), "ocr-page-000001.tif");
  QFile staged(stagedFiles.front());
  BOOST_REQUIRE(staged.open(QIODevice::ReadOnly));
  BOOST_CHECK_EQUAL(staged.readAll().toStdString(), "TIFF test page");
}

BOOST_AUTO_TEST_SUITE_END()
