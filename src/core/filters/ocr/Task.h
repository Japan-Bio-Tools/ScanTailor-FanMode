// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_TASK_H_
#define SCANTAILOR_OCR_TASK_H_

#include <QImage>
#include <memory>

#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageId.h"

class TaskStatus;

namespace ocr {
class Filter;

class Task {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(std::shared_ptr<Filter> filter, const PageId& pageId, bool batch);

  FilterResultPtr process(const TaskStatus& status, const QImage& outputImage);

 private:
  class UiUpdater;

  std::shared_ptr<Filter> m_filter;
  PageId m_pageId;
  bool m_batch;
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_TASK_H_
