// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByNaturalProvider.h"

#include <limits>
#include <utility>

#include "PageInfo.h"
#include "PageSequence.h"
#include "ProjectPages.h"

namespace output {
OrderByNaturalProvider::OrderByNaturalProvider(std::shared_ptr<ProjectPages> pages, const bool reversed)
    : m_pages(std::move(pages)), m_reversed(reversed), m_indexedImageCount(-1) {}

void OrderByNaturalProvider::rebuildIndex() const {
  m_imageOrder.clear();
  if (!m_pages) {
    m_indexedImageCount = 0;
    return;
  }

  int rank = 0;
  for (const PageInfo& page : m_pages->toPageSequence(IMAGE_VIEW)) {
    m_imageOrder.emplace(page.imageId(), rank++);
  }
  m_indexedImageCount = m_pages->numImages();
}

int OrderByNaturalProvider::imageRank(const ImageId& imageId) const {
  if (!m_pages) {
    return std::numeric_limits<int>::max();
  }
  if ((m_indexedImageCount != m_pages->numImages()) || m_imageOrder.empty()) {
    rebuildIndex();
  }
  auto found = m_imageOrder.find(imageId);
  if (found == m_imageOrder.end()) {
    // Paths may have been relinked without changing the image count.
    rebuildIndex();
    found = m_imageOrder.find(imageId);
  }
  return found == m_imageOrder.end() ? std::numeric_limits<int>::max() : found->second;
}

int OrderByNaturalProvider::subPageRank(const PageId& pageId) const {
  if (pageId.subPage() == PageId::SINGLE_PAGE) {
    return 0;
  }
  const bool rtl = m_pages && (m_pages->layoutDirection() == Qt::RightToLeft);
  if (rtl) {
    return pageId.subPage() == PageId::RIGHT_PAGE ? 0 : 1;
  }
  return pageId.subPage() == PageId::LEFT_PAGE ? 0 : 1;
}

bool OrderByNaturalProvider::precedes(const PageId& lhsPage,
                                      const bool lhsIncomplete,
                                      const PageId& rhsPage,
                                      const bool rhsIncomplete) const {
  Q_UNUSED(lhsIncomplete)
  Q_UNUSED(rhsIncomplete)
  const int lhsImageRank = imageRank(lhsPage.imageId());
  const int rhsImageRank = imageRank(rhsPage.imageId());
  if (lhsImageRank != rhsImageRank) {
    return m_reversed ? (lhsImageRank > rhsImageRank) : (lhsImageRank < rhsImageRank);
  }

  const int lhsSubPageRank = subPageRank(lhsPage);
  const int rhsSubPageRank = subPageRank(rhsPage);
  return m_reversed ? (lhsSubPageRank > rhsSubPageRank) : (lhsSubPageRank < rhsSubPageRank);
}

std::shared_ptr<const PageOrderProvider> OrderByNaturalProvider::reversed() const {
  return std::make_shared<OrderByNaturalProvider>(m_pages, !m_reversed);
}
}  // namespace output
