// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_FILTER_H_
#define SCANTAILOR_OCR_FILTER_H_

#include <QCoreApplication>
#include <memory>

#include "AbstractFilter.h"
#include "OutputFileNameGenerator.h"
#include "SafeDeletingQObjectPtr.h"

class AbstractRelinker;
class OutputFileNameGenerator;
class PageInfo;
class ProjectPages;
class ProjectReader;
class ProjectWriter;
class QDomDocument;
class QDomElement;
class ThumbnailPixmapCache;

namespace output {
class Filter;
}

namespace ocr {
class OptionsWidget;
class Settings;
class Task;

class Filter : public AbstractFilter {
  Q_DECLARE_TR_FUNCTIONS(ocr::Filter)
 public:
  Filter(std::shared_ptr<ProjectPages> pages, std::weak_ptr<output::Filter> outputFilter);

  ~Filter() override;

  QString getName() const override;

  PageView getView() const override;

  void performRelinking(const AbstractRelinker& relinker) override;

  void preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) override;

  QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& document) const override;

  void loadSettings(const ProjectReader& reader, const QDomElement& filtersElement) override;

  void loadDefaultSettings(const PageInfo& pageInfo) override;

  std::shared_ptr<Task> createTask(const PageId& pageId, bool batch);

  OptionsWidget* optionsWidget();

  void setExportContext(const OutputFileNameGenerator& outputFileNameGenerator, const QString& projectFilePath);

  std::vector<PageOrderOption> pageOrderOptions() const override;

  int selectedPageOrder() const override;

  void selectPageOrder(int option) override;

 private:
  std::shared_ptr<ProjectPages> m_pages;
  std::weak_ptr<output::Filter> m_outputFilter;
  std::shared_ptr<Settings> m_settings;
  SafeDeletingQObjectPtr<OptionsWidget> m_optionsWidget;
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_FILTER_H_
