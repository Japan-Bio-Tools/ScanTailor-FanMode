// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_ORDERBYNATURALPROVIDER_H_
#define SCANTAILOR_OUTPUT_ORDERBYNATURALPROVIDER_H_

#include <memory>
#include <unordered_map>

#include "ImageId.h"
#include "PageOrderProvider.h"

class ProjectPages;

namespace output {
/** Keeps pages in the project's true numeric order, including RTL split-page order. */
class OrderByNaturalProvider : public PageOrderProvider {
 public:
  explicit OrderByNaturalProvider(std::shared_ptr<ProjectPages> pages, bool reversed = false);

  bool precedes(const PageId& lhsPage,
                bool lhsIncomplete,
                const PageId& rhsPage,
                bool rhsIncomplete) const override;

  std::shared_ptr<const PageOrderProvider> reversed() const override;

 private:
  void rebuildIndex() const;

  int imageRank(const ImageId& imageId) const;

  int subPageRank(const PageId& pageId) const;

  std::shared_ptr<ProjectPages> m_pages;
  bool m_reversed;
  mutable int m_indexedImageCount;
  mutable std::unordered_map<ImageId, int> m_imageOrder;
};
}  // namespace output

#endif  // SCANTAILOR_OUTPUT_ORDERBYNATURALPROVIDER_H_
