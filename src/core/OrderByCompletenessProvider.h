// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
#define SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_

#include <memory>

#include "PageOrderProvider.h"

class OrderByCompletenessProvider : public PageOrderProvider {
 public:
  explicit OrderByCompletenessProvider(std::shared_ptr<const PageOrderProvider> fallback = nullptr);

  bool precedes(const PageId& lhsPage,
                bool lhsIncomplete,
                const PageId& rhsPage,
                bool rhsIncomplete) const override;

 private:
  std::shared_ptr<const PageOrderProvider> m_fallback;
};


#endif  // SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
