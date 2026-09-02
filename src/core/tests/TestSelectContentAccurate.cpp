#include <BinaryImage.h>
#include <filters/select_content/ContentBoxFinder.h>
#include <filters/select_content/Dependencies.h>
#include <filters/select_content/Params.h>

#include <QDomDocument>
#include <QPolygonF>
#include <boost/test/unit_test.hpp>

namespace Tests {
using namespace imageproc;
using namespace select_content;

namespace {
void drawGlyph(BinaryImage& image, const int x, const int y, const int width, const int height) {
  image.fill(QRect(x, y, 3, height), BLACK);
  image.fill(QRect(x, y + height - 3, width, 3), BLACK);
}
}  // namespace

BOOST_AUTO_TEST_SUITE(SelectContentAccurateTestSuite)

BOOST_AUTO_TEST_CASE(accurate_edge_pass_includes_header_and_page_number) {
  BinaryImage image(1200, 1800, WHITE);
  const QRect pageRect(0, 0, 1200, 1800);
  const QRect standardRect(180, 400, 840, 1100);

  for (int x = 470; x <= 650; x += 30) {
    drawGlyph(image, x, 150, 18, 22);
  }
  drawGlyph(image, 596, 1660, 11, 20);

  const QRect accurateRect = ContentBoxFinder::expandContentBoxForEdgeText(image, pageRect, standardRect, true, true);

  BOOST_CHECK_LT(accurateRect.top(), standardRect.top());
  BOOST_CHECK_LE(accurateRect.top(), 150);
  BOOST_CHECK_GT(accurateRect.bottom(), standardRect.bottom());
  BOOST_CHECK_GE(accurateRect.bottom(), 1679);
}

BOOST_AUTO_TEST_CASE(accurate_edge_pass_respects_disabled_protection) {
  BinaryImage image(1200, 1800, WHITE);
  const QRect pageRect(0, 0, 1200, 1800);
  const QRect standardRect(180, 400, 840, 1100);
  drawGlyph(image, 596, 1660, 11, 20);

  const QRect unchanged = ContentBoxFinder::expandContentBoxForEdgeText(image, pageRect, standardRect, false, false);
  BOOST_CHECK(unchanged == standardRect);
}

BOOST_AUTO_TEST_CASE(accurate_edge_pass_rejects_dust_and_rules) {
  BinaryImage image(1200, 1800, WHITE);
  const QRect pageRect(0, 0, 1200, 1800);
  const QRect standardRect(180, 400, 840, 1100);
  image.fill(QRect(500, 120, 2, 2), BLACK);
  image.fill(QRect(350, 1670, 220, 2), BLACK);

  const QRect unchanged = ContentBoxFinder::expandContentBoxForEdgeText(image, pageRect, standardRect, true, true);
  BOOST_CHECK(unchanged == standardRect);
}

BOOST_AUTO_TEST_CASE(accurate_option_changes_invalidate_auto_content) {
  QPolygonF pageOutline;
  pageOutline << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 150) << QPointF(0, 150);

  Dependencies standard(pageOutline, MODE_AUTO, MODE_DISABLED, false, false, true, true);
  Dependencies accurate(pageOutline, MODE_AUTO, MODE_DISABLED, false, true, true, true);
  bool updateContent = false;
  bool updatePage = false;
  accurate.compatibleWith(standard, &updateContent, &updatePage);
  BOOST_CHECK(updateContent);
  BOOST_CHECK(!updatePage);

  Dependencies withoutPageNumber(pageOutline, MODE_AUTO, MODE_DISABLED, false, true, true, false);
  accurate.compatibleWith(withoutPageNumber, &updateContent, &updatePage);
  BOOST_CHECK(updateContent);
  BOOST_CHECK(!updatePage);
}

BOOST_AUTO_TEST_CASE(accurate_options_round_trip_in_project_xml) {
  Params original(QRectF(10, 20, 300, 500), QSizeF(100, 160), QRectF(0, 0, 400, 600), Dependencies(), MODE_AUTO,
                  MODE_DISABLED, false, true, false, true);
  QDomDocument doc;
  const QDomElement element = original.toXml(doc, "params");
  doc.appendChild(element);

  const Params restored(doc.documentElement());
  BOOST_CHECK(restored.isAccurateContentDetectionEnabled());
  BOOST_CHECK(!restored.isHeaderTitleIncluded());
  BOOST_CHECK(restored.isPageNumberIncluded());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
