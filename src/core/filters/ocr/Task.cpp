// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Task.h"

#include "BasicImageView.h"
#include "Filter.h"
#include "FilterUiInterface.h"
#include "ImageViewBase.h"
#include "OptionsWidget.h"
#include "TaskStatus.h"

namespace ocr {
class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(std::shared_ptr<Filter> filter, const PageId& pageId, const QImage& image, const bool batch)
      : m_filter(std::move(filter)),
        m_pageId(pageId),
        m_image(image),
        m_downscaledImage(ImageViewBase::createDownscaledImage(image)),
        m_batch(batch) {}

  void updateUI(FilterUiInterface* ui) override {
    ui->invalidateThumbnail(m_pageId);
    if (m_batch) {
      return;
    }

    OptionsWidget* const options = m_filter->optionsWidget();
    options->postUpdateUI();
    ui->setOptionsWidget(options, ui->KEEP_OWNERSHIP);
    ui->setImageWidget(new BasicImageView(m_image, m_downscaledImage), ui->TRANSFER_OWNERSHIP);
  }

  std::shared_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  std::shared_ptr<Filter> m_filter;
  PageId m_pageId;
  QImage m_image;
  QImage m_downscaledImage;
  bool m_batch;
};

Task::Task(std::shared_ptr<Filter> filter, const PageId& pageId, const bool batch)
    : m_filter(std::move(filter)), m_pageId(pageId), m_batch(batch) {}

FilterResultPtr Task::process(const TaskStatus& status, const QImage& outputImage) {
  status.throwIfCancelled();
  return std::make_shared<UiUpdater>(m_filter, m_pageId, outputImage, m_batch);
}
}  // namespace ocr
