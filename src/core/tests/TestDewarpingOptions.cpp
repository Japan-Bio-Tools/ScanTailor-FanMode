// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/output/DewarpingOptions.h>

#include <QDomDocument>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(DewarpingOptionsTestSuite)

BOOST_AUTO_TEST_CASE(book_auto_mode_round_trips_through_project_xml) {
  output::DewarpingOptions options(output::AUTO_BOOK, true);
  options.setPostDeskewAngle(1.25);

  QDomDocument document;
  const QDomElement element = options.toXml(document, "dewarping-options");
  const output::DewarpingOptions restored(element);

  BOOST_CHECK(restored.dewarpingMode() == output::AUTO_BOOK);
  BOOST_CHECK(restored.needPostDeskew());
  BOOST_CHECK_EQUAL(restored.getPostDeskewAngle(), 1.25);
}

BOOST_AUTO_TEST_CASE(post_deskew_angle_participates_in_change_detection) {
  output::DewarpingOptions first(output::MANUAL, true);
  output::DewarpingOptions second(output::MANUAL, true);
  second.setPostDeskewAngle(0.75);

  BOOST_CHECK(first != second);
}

BOOST_AUTO_TEST_CASE(table_auto_mode_round_trips_through_project_xml) {
  output::DewarpingOptions options(output::AUTO_TABLE, true);

  QDomDocument document;
  const QDomElement element = options.toXml(document, "dewarping-options");
  BOOST_CHECK_EQUAL(element.attribute("mode").toStdString(), "auto_table");

  const output::DewarpingOptions restored(element);
  BOOST_CHECK(restored.dewarpingMode() == output::AUTO_TABLE);
}

BOOST_AUTO_TEST_SUITE_END()
