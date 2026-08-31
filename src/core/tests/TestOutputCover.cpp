// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageMetadata.h>
#include <OrderByCompletenessProvider.h>
#include <PageId.h>
#include <ProjectPages.h>
#include <filters/output/OrderByCoverProvider.h>
#include <filters/output/OrderByNaturalProvider.h>
#include <filters/output/Params.h>
#include <filters/output/Settings.h>

#include <QDomDocument>
#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_AUTO_TEST_SUITE(OutputCoverTestSuite)

BOOST_AUTO_TEST_CASE(params_round_trip_cover_settings) {
  output::Params params;
  params.setCoverRole(output::CoverRole::FRONT);
  params.setStretchCover(true);

  QDomDocument doc;
  const QDomElement element = params.toXml(doc, "params");
  const output::Params restored(element);

  BOOST_CHECK(restored.coverRole() == output::CoverRole::FRONT);
  BOOST_CHECK(restored.stretchCover());
}

BOOST_AUTO_TEST_CASE(settings_keep_cover_roles_unique) {
  output::Settings settings;
  const PageId first(ImageId("/first.tif"));
  const PageId replacement(ImageId("/replacement.tif"));

  settings.setCoverRole(first, output::CoverRole::FRONT);
  settings.setCoverRole(replacement, output::CoverRole::FRONT);

  BOOST_CHECK(settings.getParams(first).coverRole() == output::CoverRole::NONE);
  BOOST_CHECK(settings.getParams(replacement).coverRole() == output::CoverRole::FRONT);
}

BOOST_AUTO_TEST_CASE(provider_places_front_first_and_back_last) {
  auto settings = std::make_shared<output::Settings>();
  const PageId front(ImageId("/front.tif"));
  const PageId body(ImageId("/body.tif"));
  const PageId back(ImageId("/back.tif"));
  settings->setCoverRole(front, output::CoverRole::FRONT);
  settings->setCoverRole(back, output::CoverRole::BACK);

  output::OrderByCoverProvider provider(settings);
  BOOST_CHECK(provider.precedes(front, false, body, false));
  BOOST_CHECK(provider.precedes(body, false, back, false));
  BOOST_CHECK(provider.precedes(front, false, back, false));
  BOOST_CHECK(!provider.precedes(body, false, front, false));
  BOOST_CHECK(!provider.precedes(back, false, body, false));

  const auto reversed = provider.reversed();
  BOOST_CHECK(reversed->precedes(front, false, body, false));
  BOOST_CHECK(reversed->precedes(body, false, back, false));
}

BOOST_AUTO_TEST_CASE(natural_provider_keeps_project_number_order_after_refresh) {
  std::vector<ImageInfo> images;
  images.emplace_back(ImageId("/book/page-1.tif"), ImageMetadata(), 1, false, false);
  images.emplace_back(ImageId("/book/page-2.tif"), ImageMetadata(), 1, false, false);
  images.emplace_back(ImageId("/book/page-10.tif"), ImageMetadata(), 1, false, false);
  auto pages = std::make_shared<ProjectPages>(images, Qt::RightToLeft);
  auto natural = std::make_shared<output::OrderByNaturalProvider>(pages);
  const PageId page1(images[0].id());
  const PageId page2(images[1].id());
  const PageId page10(images[2].id());

  BOOST_CHECK(natural->precedes(page1, false, page2, false));
  BOOST_CHECK(natural->precedes(page2, false, page10, false));
  BOOST_CHECK(!natural->precedes(page10, false, page2, false));

  const OrderByCompletenessProvider completeness(natural);
  BOOST_CHECK(completeness.precedes(page2, true, page10, true));
  BOOST_CHECK(!completeness.precedes(page10, true, page2, true));
  BOOST_CHECK(completeness.precedes(page10, false, page1, true));
}

BOOST_AUTO_TEST_SUITE_END()
