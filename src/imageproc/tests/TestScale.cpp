// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <GrayImage.h>
#include <Scale.h>

#include <QSize>
#include <boost/test/unit_test.hpp>
#include <cstdint>

namespace imageproc {
namespace tests {
BOOST_AUTO_TEST_SUITE(ScaleTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const GrayImage nullImg;
  BOOST_CHECK(scaleToGray(nullImg, QSize(1, 1)).isNull());
}

static void setPixels(GrayImage& image, const uint8_t* pixels) {
  for (int y = 0; y < image.height(); ++y) {
    uint8_t* line = image.data() + y * image.stride();
    for (int x = 0; x < image.width(); ++x) {
      line[x] = pixels[y * image.width() + x];
    }
  }
}

static void checkPixels(const GrayImage& image, const uint8_t* expected) {
  for (int y = 0; y < image.height(); ++y) {
    const uint8_t* line = image.data() + y * image.stride();
    for (int x = 0; x < image.width(); ++x) {
      BOOST_CHECK_EQUAL(line[x], expected[y * image.width() + x]);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_integer_downscale_averages_blocks) {
  GrayImage image(QSize(4, 4));
  const uint8_t pixels[] = {
      0, 10, 40, 50,
      20, 30, 60, 70,
      80, 90, 120, 130,
      100, 110, 140, 150};
  setPixels(image, pixels);

  const GrayImage scaled(scaleToGray(image, QSize(2, 2)));
  const uint8_t expected[] = {15, 55, 95, 135};
  checkPixels(scaled, expected);
}

BOOST_AUTO_TEST_CASE(test_non_uniform_integer_upscale_repeats_pixels) {
  GrayImage image(QSize(2, 2));
  const uint8_t pixels[] = {1, 2, 3, 4};
  setPixels(image, pixels);

  const GrayImage scaled(scaleToGray(image, QSize(4, 6)));
  const uint8_t expected[] = {
      1, 1, 2, 2,
      1, 1, 2, 2,
      1, 1, 2, 2,
      3, 3, 4, 4,
      3, 3, 4, 4,
      3, 3, 4, 4};
  checkPixels(scaled, expected);
}

BOOST_AUTO_TEST_CASE(test_generic_downscale_preserves_constant_image) {
  GrayImage image(QSize(100, 100));
  for (int y = 0; y < image.height(); ++y) {
    uint8_t* line = image.data() + y * image.stride();
    for (int x = 0; x < image.width(); ++x) {
      line[x] = 123;
    }
  }

  const GrayImage scaled(scaleToGray(image, QSize(80, 80)));
  BOOST_CHECK_EQUAL(scaled.size().width(), 80);
  BOOST_CHECK_EQUAL(scaled.size().height(), 80);
  for (int y = 0; y < scaled.height(); ++y) {
    const uint8_t* line = scaled.data() + y * scaled.stride();
    for (int x = 0; x < scaled.width(); ++x) {
      BOOST_CHECK_EQUAL(line[x], 123);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc
