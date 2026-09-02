// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_OPTIONSWIDGET_H_
#define SCANTAILOR_OCR_OPTIONSWIDGET_H_

#include <memory>
#include <QStringList>
#include <vector>

#include "FilterOptionsWidget.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "ui_OcrOptionsWidget.h"

class AbstractRelinker;
class ProjectPages;

namespace output {
class Filter;
}

namespace ocr {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OcrOptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(std::shared_ptr<Settings> settings,
                std::shared_ptr<ProjectPages> pages,
                std::weak_ptr<output::Filter> outputFilter);

  void preUpdateUI(const PageId& pageId);

  void postUpdateUI();

  void syncFromSettings();

  void setExportContext(const OutputFileNameGenerator& outputFileNameGenerator, const QString& projectFilePath);

  void performRelinking(const AbstractRelinker& relinker);

 private slots:
  void languagesChanged();

  void compressionChanged(int index);

  void exportPdf();

 private:
  std::vector<PageId> orderedPageIds() const;

  QStringList outputFiles(int* missingCount = nullptr, qint64* totalBytes = nullptr) const;

  void updateEstimate();

  QString selectedLanguages() const;

  QString suggestedPdfPath(bool searchable) const;

  static QString formatBytes(qint64 bytes);

  static QString findBundledTool(const QString& toolName);

  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ProjectPages> m_pages;
  std::weak_ptr<output::Filter> m_outputFilter;
  OutputFileNameGenerator m_outputFileNameGenerator;
  QString m_projectFilePath;
  PageId m_pageId;
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_OPTIONSWIDGET_H_
