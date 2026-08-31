// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_ORDERBYCOVERPROVIDER_H_
#define SCANTAILOR_OUTPUT_ORDERBYCOVERPROVIDER_H_

#include <memory>

#include "PageOrderProvider.h"

namespace output {
class Settings;

/** Keeps the front cover first and the back cover last, with an optional fallback order. */
class OrderByCoverProvider : public PageOrderProvider {
 public:
  explicit OrderByCoverProvider(std::shared_ptr<Settings> settings,
                                std::shared_ptr<const PageOrderProvider> fallback = nullptr);

  bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override;

  std::shared_ptr<const PageOrderProvider> reversed() const override;

 private:
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<const PageOrderProvider> m_fallback;
};
}  // namespace output

#endif  // SCANTAILOR_OUTPUT_ORDERBYCOVERPROVIDER_H_
