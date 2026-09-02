// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_ux_qt.h"

#include <CollapsibleGroupBox.h>
#include <ErrorWidget.h>
#include <ImageId.h>
#include <PageId.h>
#include <PageRange.h>
#include <PageSelectionAccessor.h>
#include <PageSelectionProvider.h>
#include <PageSequence.h>
#include <filters/output/OptionsWidget.h>
#include <filters/output/ChangeDewarpingDialog.h>
#include <filters/output/Settings.h>
#include <filters/ocr/OptionsWidget.h>
#include <filters/ocr/Settings.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QWidget>
#include <QtTest/QtTest>
#include <memory>
#include <set>
#include <vector>

namespace {
class EmptyPageSelectionProvider final : public PageSelectionProvider {
 public:
  PageSequence allPages() const override { return PageSequence(); }

  std::set<PageId> selectedPages() const override { return {}; }

  std::vector<PageRange> selectedRanges() const override { return {}; }
};
}  // namespace

void TestUxQt::testButtonClickEmitsSignal() {
  QPushButton button("Click me");
  QSignalSpy spy(&button, &QPushButton::clicked);
  QVERIFY(spy.isValid());

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 1);

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 2);
}

void TestUxQt::testCheckBoxToggleChangesState() {
  QCheckBox box("Option");
  QVERIFY(!box.isChecked());

  box.setChecked(true);
  QVERIFY(box.isChecked());

  box.setChecked(false);
  QVERIFY(!box.isChecked());

  QSignalSpy spy(&box, &QCheckBox::toggled);
  box.setChecked(true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(box.isChecked());
}

void TestUxQt::testLineEditAcceptsInput() {
  QLineEdit edit;
  QCOMPARE(edit.text(), QString());

  QTest::keyClicks(&edit, "Hello UX");
  QCOMPARE(edit.text(), QString("Hello UX"));

  edit.clear();
  QTest::keyClicks(&edit, "123");
  QCOMPARE(edit.text(), QString("123"));
}

void TestUxQt::testCollapsibleGroupBoxStateAndSignal() {
  CollapsibleGroupBox group("Options");
  QSignalSpy spy(&group, &CollapsibleGroupBox::collapsedStateChanged);
  QVERIFY(spy.isValid());

  QVERIFY(!group.isCollapsed());

  group.setCollapsed(true);
  QVERIFY(group.isCollapsed());
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);

  group.setCollapsed(false);
  QVERIFY(!group.isCollapsed());
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toBool(), false);
}

void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}

void TestUxQt::testDewarpingControlsAndSignals() {
  auto settings = std::make_shared<output::Settings>();
  const auto provider = std::make_shared<EmptyPageSelectionProvider>();
  output::OptionsWidget widget(settings, PageSelectionAccessor(provider));
  widget.preUpdateUI(PageId(ImageId("dewarping-test.tif")));

  auto* liveChange = widget.findChild<QCheckBox*>("liveDewarpingPreviewCB");
  auto* resetGrid = widget.findChild<QPushButton*>("resetDewarpingButton");
  auto* redetect = widget.findChild<QPushButton*>("redetectDewarpingButton");
  auto* editMode = widget.findChild<QComboBox*>("dewarpingEditModeCombo");
  QVERIFY(liveChange != nullptr);
  QVERIFY(resetGrid != nullptr);
  QVERIFY(redetect != nullptr);
  QVERIFY(editMode != nullptr);
  QVERIFY(liveChange->isChecked());
  QCOMPARE(editMode->count(), 2);
  QVERIFY(editMode->currentData().toBool());

  QSignalSpy simpleModeSpy(&widget, &output::OptionsWidget::simpleDewarpingModeChanged);
  editMode->setCurrentIndex(1);
  QCOMPARE(simpleModeSpy.count(), 1);
  QCOMPARE(simpleModeSpy.at(0).at(0).toBool(), false);
  editMode->setCurrentIndex(0);
  QCOMPARE(simpleModeSpy.count(), 2);
  QCOMPARE(simpleModeSpy.at(1).at(0).toBool(), true);

  QSignalSpy liveSpy(&widget, &output::OptionsWidget::liveDewarpingPreviewChanged);
  QSignalSpy resetSpy(&widget, &output::OptionsWidget::resetDewarpingRequested);
  liveChange->setChecked(false);
  QCOMPARE(liveSpy.count(), 1);
  QCOMPARE(liveSpy.at(0).at(0).toBool(), false);

  QTest::mouseClick(resetGrid, Qt::LeftButton);
  QCOMPARE(resetSpy.count(), 1);
  QCOMPARE(settings->getParams(PageId(ImageId("dewarping-test.tif"))).dewarpingOptions().dewarpingMode(),
           output::MANUAL);

  QSignalSpy reloadSpy(&widget, &output::OptionsWidget::reloadRequested);
  QSignalSpy thumbnailSpy(&widget, SIGNAL(invalidateThumbnail(PageId)));
  QVERIFY(thumbnailSpy.isValid());
  QTest::mouseClick(redetect, Qt::LeftButton);
  const output::Params redetectedParams(settings->getParams(PageId(ImageId("dewarping-test.tif"))));
  QCOMPARE(redetectedParams.dewarpingOptions().dewarpingMode(), output::AUTO_BOOK);
  QVERIFY(!redetectedParams.distortionModel().isValid());
  QCOMPARE(reloadSpy.count(), 1);
  QCOMPARE(thumbnailSpy.count(), 1);
}

void TestUxQt::testTableDewarpingModeIsAvailable() {
  const auto provider = std::make_shared<EmptyPageSelectionProvider>();
  const PageId pageId(ImageId("table-test.tif"));
  const output::DewarpingOptions options(output::AUTO_TABLE, true);
  output::ChangeDewarpingDialog dialog(nullptr, pageId, options, PageSelectionAccessor(provider));

  auto* tableMode = dialog.findChild<QRadioButton*>("autoTableRB");
  QVERIFY(tableMode != nullptr);
  QVERIFY(tableMode->isChecked());
  QVERIFY(tableMode->text().contains(QStringLiteral("Tables")));
}

void TestUxQt::testPdfExportAllowsNoOcrLanguages() {
  auto settings = std::make_shared<ocr::Settings>();
  settings->setArabicEnabled(false);
  settings->setEnglishEnabled(false);
  ocr::OptionsWidget widget(settings, std::shared_ptr<ProjectPages>(), std::weak_ptr<output::Filter>());

  auto* arabic = widget.findChild<QCheckBox*>("arabicCheckBox");
  auto* english = widget.findChild<QCheckBox*>("englishCheckBox");
  auto* exportButton = widget.findChild<QPushButton*>("exportPdfButton");
  QVERIFY(arabic != nullptr);
  QVERIFY(english != nullptr);
  QVERIFY(exportButton != nullptr);
  QVERIFY(!arabic->isChecked());
  QVERIFY(!english->isChecked());
  QVERIFY(exportButton->text().contains(QStringLiteral("without OCR"), Qt::CaseInsensitive));

  english->setChecked(true);
  QVERIFY(exportButton->text().contains(QStringLiteral("Searchable"), Qt::CaseInsensitive));
}
