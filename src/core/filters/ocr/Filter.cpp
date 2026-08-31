// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include "AbstractRelinker.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "filters/output/Filter.h"

namespace ocr {
Filter::Filter(std::shared_ptr<ProjectPages> pages, std::weak_ptr<output::Filter> outputFilter)
    : m_pages(std::move(pages)),
      m_outputFilter(std::move(outputFilter)),
      m_settings(std::make_shared<Settings>()),
      m_optionsWidget(new OptionsWidget(m_settings, m_pages, m_outputFilter)) {}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("ocr::Filter", "OCR & PDF");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_optionsWidget->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) {
  m_optionsWidget->preUpdateUI(pageInfo.id());
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& document) const {
  Q_UNUSED(writer)
  return m_settings->toXml(document, QStringLiteral("ocr"));
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersElement) {
  Q_UNUSED(reader)
  *m_settings = Settings(filtersElement.namedItem(QStringLiteral("ocr")).toElement());
  m_optionsWidget->syncFromSettings();
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  Q_UNUSED(pageInfo)
}

std::shared_ptr<Task> Filter::createTask(const PageId& pageId, const bool batch) {
  return std::make_shared<Task>(std::static_pointer_cast<Filter>(shared_from_this()), pageId, batch);
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}

void Filter::setExportContext(const OutputFileNameGenerator& outputFileNameGenerator,
                              const QString& projectFilePath) {
  m_optionsWidget->setExportContext(outputFileNameGenerator, projectFilePath);
}

std::vector<PageOrderOption> Filter::pageOrderOptions() const {
  const std::shared_ptr<output::Filter> filter = m_outputFilter.lock();
  return filter ? filter->pageOrderOptions() : std::vector<PageOrderOption>();
}

int Filter::selectedPageOrder() const {
  const std::shared_ptr<output::Filter> filter = m_outputFilter.lock();
  return filter ? filter->selectedPageOrder() : -1;
}

void Filter::selectPageOrder(const int option) {
  const std::shared_ptr<output::Filter> filter = m_outputFilter.lock();
  if (filter) {
    filter->selectPageOrder(option);
  }
}
}  // namespace ocr
