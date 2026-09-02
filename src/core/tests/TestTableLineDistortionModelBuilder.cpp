// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <GrayImage.h>
#include <TableLineDistortionModelBuilder.h>
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <cmath>

using dewarping::TableLineDistortionModelBuilder;
using imageproc::GrayImage;

namespace {
void putDark(GrayImage& image, const int x, const int y) {
  if (image.rect().contains(x, y)) {
    image.data()[y * image.stride() + x] = 25;
  }
}

double tableY(const int x, const int base) {
  const double normalizedX = (x - 450.0) / 330.0;
  return base - 0.014 * (x - 450.0) + 5.0 * normalizedX * normalizedX;
}

void drawTableRule(GrayImage& image, const int base) {
  for (int x = 120; x <= 780; ++x) {
    const int y = qRound(tableY(x, base));
    putDark(image, x, y - 1);
    putDark(image, x, y);
    putDark(image, x, y + 1);
  }
}

void drawBrokenTextRows(GrayImage& image) {
  for (int y = 135; y < 1080; y += 37) {
    for (int x = 160; x < 740; x += 49) {
      for (int xx = x; xx < std::min(x + 25, 740); ++xx) {
        putDark(image, xx, y);
      }
    }
  }
}

double maxDeviationFromChord(const std::vector<QPointF>& points) {
  const QPointF chord = points.back() - points.front();
  const double length = std::hypot(chord.x(), chord.y());
  double deviation = 0.0;
  for (const QPointF& point : points) {
    const QPointF delta = point - points.front();
    deviation = std::max(deviation, std::abs(chord.x() * delta.y() - chord.y() * delta.x()) / length);
  }
  return deviation;
}
}  // namespace

BOOST_AUTO_TEST_SUITE(TableLineDistortionModelBuilderTestSuite)

BOOST_AUTO_TEST_CASE(detects_a_curved_family_of_table_rules) {
  GrayImage image(QSize(900, 1200));
  image.fill(245);
  drawBrokenTextRows(image);
  for (const int base : {190, 365, 590, 815, 1030}) {
    drawTableRule(image, base);
  }

  const TableLineDistortionModelBuilder::Result result
      = TableLineDistortionModelBuilder::build(image, QRect(80, 80, 740, 1040));

  BOOST_REQUIRE(result.reliable);
  BOOST_CHECK(result.model.isValid());
  BOOST_CHECK_GE(result.acceptedLineCount, 5);
  BOOST_CHECK_GT(maxDeviationFromChord(result.model.topCurve().polyline()), 1.0);
  BOOST_CHECK_GT(maxDeviationFromChord(result.model.bottomCurve().polyline()), 1.0);
}

BOOST_AUTO_TEST_CASE(rejects_broken_text_strokes_without_table_rules) {
  GrayImage image(QSize(900, 1200));
  image.fill(245);
  drawBrokenTextRows(image);

  const TableLineDistortionModelBuilder::Result result
      = TableLineDistortionModelBuilder::build(image, QRect(80, 80, 740, 1040));

  BOOST_CHECK(!result.reliable);
  BOOST_CHECK(!result.model.isValid());
}

BOOST_AUTO_TEST_SUITE_END()
