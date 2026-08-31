// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ProjectReader.h>

#include <QDomDocument>
#include <boost/test/unit_test.hpp>

namespace {
QDomDocument projectDocument(const int version) {
  QDomDocument doc;
  const QString xml = QString::fromLatin1(
                          "<project version=\"%1\" outputDirectory=\"out\">"
                          "<directories><directory id=\"1\" path=\"C:/book\"/></directories>"
                          "<files><file id=\"2\" dirId=\"1\" name=\"page.tif\"/></files>"
                          "<images><image id=\"3\" subPages=\"1\" fileId=\"2\" fileImage=\"0\">"
                          "<size width=\"100\" height=\"200\"/>"
                          "<dpi horizontal=\"300\" vertical=\"300\"/>"
                          "</image></images>"
                          "<pages><page id=\"4\" imageId=\"3\" subPage=\"single\"/></pages>"
                          "<file-name-disambiguation/>"
                          "<filters/>"
                          "</project>")
                          .arg(version);
  BOOST_REQUIRE(doc.setContent(xml));
  return doc;
}
}  // namespace

BOOST_AUTO_TEST_SUITE(ProjectReaderCompatibilityTestSuite)

BOOST_AUTO_TEST_CASE(opens_advanced_and_legacy_projects) {
  const ProjectReader version3(projectDocument(3));
  const ProjectReader version4(projectDocument(4));
  BOOST_CHECK(version3.success());
  BOOST_CHECK(version4.success());
}

BOOST_AUTO_TEST_CASE(rejects_unknown_project_versions) {
  const ProjectReader version2(projectDocument(2));
  const ProjectReader version5(projectDocument(5));
  BOOST_CHECK(!version2.success());
  BOOST_CHECK(!version5.success());
}

BOOST_AUTO_TEST_SUITE_END()
