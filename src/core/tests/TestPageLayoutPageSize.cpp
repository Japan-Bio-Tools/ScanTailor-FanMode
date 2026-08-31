// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <PageId.h>
#include <filters/output/Settings.h>
#include <filters/page_layout/Alignment.h>
#include <filters/page_layout/Params.h>
#include <filters/page_layout/Settings.h>

#include <QDomDocument>
#include <QRectF>
#include <QSizeF>
#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_AUTO_TEST_SUITE(PageLayoutPageSizeTestSuite)

namespace {
page_layout::Params makeParams(const QSizeF& contentSizeMm,
                               const page_layout::PageSizeMode mode = page_layout::PageSizeMode::AUTO,
                               const QSizeF& customSizeMm = QSizeF()) {
  return page_layout::Params(Margins(), QRectF(), QRectF(), contentSizeMm,
                             page_layout::Alignment(page_layout::Alignment::TOP, page_layout::Alignment::HCENTER),
                             false, mode, customSizeMm);
}
}  // namespace

BOOST_AUTO_TEST_CASE(params_round_trip_page_size) {
  const page_layout::Params original
      = makeParams(QSizeF(120.0, 180.0), page_layout::PageSizeMode::CUSTOM, QSizeF(148.0, 210.0));

  QDomDocument doc;
  const QDomElement element = original.toXml(doc, "params");
  const page_layout::Params restored(element);

  BOOST_CHECK(restored.pageSizeMode() == page_layout::PageSizeMode::CUSTOM);
  BOOST_CHECK(restored.customPageSizeMM() == QSizeF(148.0, 210.0));
}

BOOST_AUTO_TEST_CASE(custom_size_is_page_specific_and_never_crops_content) {
  auto outputSettings = std::make_shared<output::Settings>();
  page_layout::Settings settings(
      [outputSettings]() { return outputSettings->coverPageId(output::CoverRole::FRONT); });
  const PageId page(ImageId("/body.tif"));
  const PageId otherPage(ImageId("/other.tif"));

  settings.setPageParams(page, makeParams(QSizeF(120.0, 180.0)));
  settings.setPageParams(otherPage, makeParams(QSizeF(110.0, 170.0)));
  settings.setPageSize(page, page_layout::PageSizeMode::CUSTOM, QSizeF(100.0, 250.0));

  BOOST_CHECK(settings.getTargetPageSizeMM(page) == QSizeF(120.0, 250.0));
  BOOST_CHECK(settings.getPageSizeMode(otherPage) == page_layout::PageSizeMode::AUTO);
}

BOOST_AUTO_TEST_CASE(front_cover_mode_uses_selected_cover_hard_size) {
  auto outputSettings = std::make_shared<output::Settings>();
  page_layout::Settings settings(
      [outputSettings]() { return outputSettings->coverPageId(output::CoverRole::FRONT); });
  const PageId front(ImageId("/front.tif"));
  const PageId body(ImageId("/body.tif"));

  settings.setPageParams(front, makeParams(QSizeF(150.0, 220.0)));
  settings.setPageParams(body, makeParams(QSizeF(100.0, 180.0)));
  settings.setPageSize(body, page_layout::PageSizeMode::FRONT_COVER, QSizeF());
  outputSettings->setCoverRole(front, output::CoverRole::FRONT);

  BOOST_CHECK(settings.hasFrontCover());
  BOOST_CHECK(settings.getTargetPageSizeMM(body) == QSizeF(150.0, 220.0));
}

BOOST_AUTO_TEST_SUITE_END()
