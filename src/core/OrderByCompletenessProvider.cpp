// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByCompletenessProvider.h"

#include <utility>

OrderByCompletenessProvider::OrderByCompletenessProvider(std::shared_ptr<const PageOrderProvider> fallback)
    : m_fallback(std::move(fallback)) {}

bool OrderByCompletenessProvider::precedes(const PageId& lhsPage,
                                           const bool lhsIncomplete,
                                           const PageId& rhsPage,
                                           const bool rhsIncomplete) const {
  if (lhsIncomplete != rhsIncomplete) {
    // Incomplete pages go to the back.
    return rhsIncomplete;
  }
  return m_fallback && m_fallback->precedes(lhsPage, lhsIncomplete, rhsPage, rhsIncomplete);
}
