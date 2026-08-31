// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByCoverProvider.h"

#include <utility>

#include "Params.h"
#include "Settings.h"

namespace output {
namespace {
int coverRank(const CoverRole role) {
  switch (role) {
    case CoverRole::FRONT:
      return 0;
    case CoverRole::BACK:
      return 2;
    case CoverRole::NONE:
    default:
      return 1;
  }
}
}  // namespace

OrderByCoverProvider::OrderByCoverProvider(std::shared_ptr<Settings> settings,
                                           std::shared_ptr<const PageOrderProvider> fallback)
    : m_settings(std::move(settings)), m_fallback(std::move(fallback)) {}

bool OrderByCoverProvider::precedes(const PageId& lhsPage,
                                    const bool lhsIncomplete,
                                    const PageId& rhsPage,
                                    const bool rhsIncomplete) const {
  const int lhsRank = coverRank(m_settings->getParams(lhsPage).coverRole());
  const int rhsRank = coverRank(m_settings->getParams(rhsPage).coverRole());
  if (lhsRank != rhsRank) {
    return lhsRank < rhsRank;
  }

  if (!m_fallback) {
    return false;
  }

  // Some historical providers consider equivalent pages to precede each
  // other. Resolve that ambiguity to preserve a strict, stable ordering.
  const bool lhsFirst = m_fallback->precedes(lhsPage, lhsIncomplete, rhsPage, rhsIncomplete);
  const bool rhsFirst = m_fallback->precedes(rhsPage, rhsIncomplete, lhsPage, lhsIncomplete);
  return lhsFirst && !rhsFirst;
}

std::shared_ptr<const PageOrderProvider> OrderByCoverProvider::reversed() const {
  // Sorting direction may reverse the body-page order, but cover semantics
  // remain fixed: front first and back last.
  return std::make_shared<OrderByCoverProvider>(m_settings, m_fallback ? m_fallback->reversed() : nullptr);
}
}  // namespace output
