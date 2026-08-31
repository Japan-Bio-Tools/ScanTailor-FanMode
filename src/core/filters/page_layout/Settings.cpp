// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"

#include <DeviationProvider.h>

#include <QMutex>
#include <boost/foreach.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <optional>
#include <utility>

#include "AbstractRelinker.h"
#include "Guide.h"
#include "PageId.h"
#include "PageSequence.h"
#include "Params.h"
#include "RelinkablePath.h"


using namespace ::boost;
using namespace ::boost::multi_index;

namespace page_layout {
class Settings::Item {
 public:
  PageId pageId;
  Margins hardMarginsMM;
  QRectF pageRect;
  QRectF contentRect;
  QSizeF contentSizeMM;
  Alignment alignment;
  bool autoMargins;
  PageSizeMode pageSizeMode;
  QSizeF customPageSizeMM;

  Item(const PageId& pageId,
       const Margins& hardMarginsMm,
       const QRectF& pageRect,
       const QRectF& contentRect,
       const QSizeF& contentSizeMm,
       const Alignment& alignment,
       bool autoMargins,
       PageSizeMode pageSizeMode = PageSizeMode::AUTO,
       const QSizeF& customPageSizeMm = QSizeF());

  double hardWidthMM() const;

  double hardHeightMM() const;

  double influenceHardWidthMM() const;

  double influenceHardHeightMM() const;

  bool alignedWithOthers() const { return !alignment.isNull(); }
};


class Settings::ModifyMargins {
 public:
  ModifyMargins(const Margins& marginsMm, const bool autoMargins)
      : m_marginsMM(marginsMm), m_autoMargins(autoMargins) {}

  void operator()(Item& item) {
    item.hardMarginsMM = m_marginsMM;
    item.autoMargins = m_autoMargins;
  }

 private:
  Margins m_marginsMM;
  bool m_autoMargins;
};


class Settings::ModifyAlignment {
 public:
  explicit ModifyAlignment(const Alignment& alignment) : m_alignment(alignment) {}

  void operator()(Item& item) { item.alignment = m_alignment; }

 private:
  Alignment m_alignment;
};


class Settings::ModifyContentSize {
 public:
  ModifyContentSize(const QSizeF& contentSizeMm, const QRectF& contentRect, const QRectF& pageRect)
      : m_contentSizeMM(contentSizeMm), m_contentRect(contentRect), m_pageRect(pageRect) {}

  void operator()(Item& item) {
    item.contentSizeMM = m_contentSizeMM;
    item.contentRect = m_contentRect;
    item.pageRect = m_pageRect;
  }

 private:
  QSizeF m_contentSizeMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
};


class Settings::ModifyPageSize {
 public:
  ModifyPageSize(const PageSizeMode mode, const QSizeF& customSizeMm)
      : m_mode(mode), m_customSizeMM(customSizeMm) {}

  void operator()(Item& item) {
    item.pageSizeMode = m_mode;
    item.customPageSizeMM = m_customSizeMM;
  }

 private:
  PageSizeMode m_mode;
  QSizeF m_customSizeMM;
};


class Settings::Impl {
 public:
  explicit Impl(FrontCoverProvider frontCoverProvider);

  ~Impl();

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void removePagesMissingFrom(const PageSequence& pages);

  bool checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const;

  std::unique_ptr<Params> getPageParams(const PageId& pageId) const;

  bool isParamsNull(const PageId& pageId) const;

  void setPageParams(const PageId& pageId, const Params& params);

  Params updateContentSizeAndGetParams(const PageId& pageId,
                                       const QRectF& pageRect,
                                       const QRectF& contentRect,
                                       const QSizeF& contentSizeMm,
                                       QSizeF* aggHardSizeBefore,
                                       QSizeF* aggHardSizeAfter);

  Margins getHardMarginsMM(const PageId& pageId) const;

  void setHardMarginsMM(const PageId& pageId, const Margins& marginsMm);

  Alignment getPageAlignment(const PageId& pageId) const;

  AggregateSizeChanged setPageAlignment(const PageId& pageId, const Alignment& alignment);

  AggregateSizeChanged setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm);

  void invalidateContentSize(const PageId& pageId);

  QSizeF getAggregateHardSizeMM() const;

  QSizeF getAggregateHardSizeMMLocked() const;

  QSizeF computeAggregateHardSizeMMLocked() const;

  QSizeF getAggregateHardSizeMM(const PageId& pageId, const QSizeF& hardSizeMm, const Alignment& alignment) const;

  void setAggregateHardSizeFrozen(bool frozen);

  bool isAggregateHardSizeFrozen() const;

  bool isPageAutoMarginsEnabled(const PageId& pageId);

  void setPageAutoMarginsEnabled(const PageId& pageId, bool state);

  PageSizeMode getPageSizeMode(const PageId& pageId) const;

  QSizeF getCustomPageSizeMM(const PageId& pageId) const;

  void setPageSize(const PageId& pageId, PageSizeMode mode, const QSizeF& customSizeMm);

  QSizeF getTargetPageSizeMM(const PageId& pageId) const;

  QSizeF getTargetPageSizeMM(const PageId& pageId,
                             const QSizeF& hardSizeMm,
                             const Alignment& alignment) const;

  bool hasFrontCover() const;

  const DeviationProvider<PageId>& deviationProvider() const;

  std::vector<Guide>& guides();

  bool isShowingMiddleRectEnabled() const;

  void enableShowingMiddleRect(bool state);

 private:
  QSizeF targetPageSizeMMLocked(const PageId& pageId,
                                const QSizeF& hardSizeMm,
                                const Alignment& alignment) const;

  QSizeF aggregateHardSizeWithOverrideMMLocked(const PageId& pageId,
                                               const QSizeF& hardSizeMm,
                                               const Alignment& alignment) const;

  static QSizeF boundedTargetSize(const QSizeF& requestedSizeMm, const QSizeF& hardSizeMm);

  class SequencedTag;
  class DescWidthTag;
  class DescHeightTag;

  using Container = multi_index_container<
      Item,
      indexed_by<hashed_unique<member<Item, PageId, &Item::pageId>, std::hash<PageId>>,
                 sequenced<tag<SequencedTag>>,
                 ordered_non_unique<tag<DescWidthTag>,
                                    // ORDER BY alignedWithOthers DESC, hardWidthMM DESC
                                    composite_key<Item,
                                                  const_mem_fun<Item, bool, &Item::alignedWithOthers>,
                                                  const_mem_fun<Item, double, &Item::hardWidthMM>>,
                                    composite_key_compare<std::greater<>, std::greater<double>>>,
                 ordered_non_unique<tag<DescHeightTag>,
                                    // ORDER BY alignedWithOthers DESC, hardHeightMM DESC
                                    composite_key<Item,
                                                  const_mem_fun<Item, bool, &Item::alignedWithOthers>,
                                                  const_mem_fun<Item, double, &Item::hardHeightMM>>,
                                    composite_key_compare<std::greater<>, std::greater<double>>>>>;

  using UnorderedItems = Container::index<SequencedTag>::type;
  using DescWidthOrder = Container::index<DescWidthTag>::type;
  using DescHeightOrder = Container::index<DescHeightTag>::type;

  mutable QMutex m_mutex;
  Container m_items;
  UnorderedItems& m_unorderedItems;
  DescWidthOrder& m_descWidthOrder;
  DescHeightOrder& m_descHeightOrder;
  const QRectF m_invalidRect;
  const QSizeF m_invalidSize;
  const Margins m_defaultHardMarginsMM;
  const Alignment m_defaultAlignment;
  const bool m_autoMarginsDefault;
  DeviationProvider<PageId> m_deviationProvider;
  std::vector<Guide> m_guides;
  bool m_showMiddleRect;
  FrontCoverProvider m_frontCoverProvider;

  std::optional<QSizeF> m_frozenAggregateHardSizeMm;
};


/*=============================== Settings ==================================*/

Settings::Settings(FrontCoverProvider frontCoverProvider)
    : m_impl(std::make_unique<Impl>(std::move(frontCoverProvider))) {}

Settings::~Settings() = default;

void Settings::clear() {
  return m_impl->clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  m_impl->performRelinking(relinker);
}

void Settings::removePagesMissingFrom(const PageSequence& pages) {
  m_impl->removePagesMissingFrom(pages);
}

bool Settings::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  return m_impl->checkEverythingDefined(pages, ignore);
}

std::unique_ptr<Params> Settings::getPageParams(const PageId& pageId) const {
  return m_impl->getPageParams(pageId);
}

void Settings::setPageParams(const PageId& pageId, const Params& params) {
  return m_impl->setPageParams(pageId, params);
}

Params Settings::updateContentSizeAndGetParams(const PageId& pageId,
                                               const QRectF& pageRect,
                                               const QRectF& contentRect,
                                               const QSizeF& contentSizeMm,
                                               QSizeF* aggHardSizeBefore,
                                               QSizeF* aggHardSizeAfter) {
  return m_impl->updateContentSizeAndGetParams(pageId, pageRect, contentRect, contentSizeMm, aggHardSizeBefore,
                                               aggHardSizeAfter);
}

Margins Settings::getHardMarginsMM(const PageId& pageId) const {
  return m_impl->getHardMarginsMM(pageId);
}

void Settings::setHardMarginsMM(const PageId& pageId, const Margins& marginsMm) {
  m_impl->setHardMarginsMM(pageId, marginsMm);
}

Alignment Settings::getPageAlignment(const PageId& pageId) const {
  return m_impl->getPageAlignment(pageId);
}

Settings::AggregateSizeChanged Settings::setPageAlignment(const PageId& pageId, const Alignment& alignment) {
  return m_impl->setPageAlignment(pageId, alignment);
}

Settings::AggregateSizeChanged Settings::setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm) {
  return m_impl->setContentSizeMM(pageId, contentSizeMm);
}

void Settings::invalidateContentSize(const PageId& pageId) {
  return m_impl->invalidateContentSize(pageId);
}

QSizeF Settings::getAggregateHardSizeMM() const {
  return m_impl->getAggregateHardSizeMM();
}

QSizeF Settings::getAggregateHardSizeMM(const PageId& pageId,
                                        const QSizeF& hardSizeMm,
                                        const Alignment& alignment) const {
  return m_impl->getAggregateHardSizeMM(pageId, hardSizeMm, alignment);
}

void Settings::setAggregateHardSizeFrozen(const bool frozen) {
  m_impl->setAggregateHardSizeFrozen(frozen);
}

bool Settings::isAggregateHardSizeFrozen() const {
  return m_impl->isAggregateHardSizeFrozen();
}

bool Settings::isPageAutoMarginsEnabled(const PageId& pageId) {
  return m_impl->isPageAutoMarginsEnabled(pageId);
}

void Settings::setPageAutoMarginsEnabled(const PageId& pageId, const bool state) {
  return m_impl->setPageAutoMarginsEnabled(pageId, state);
}

PageSizeMode Settings::getPageSizeMode(const PageId& pageId) const {
  return m_impl->getPageSizeMode(pageId);
}

QSizeF Settings::getCustomPageSizeMM(const PageId& pageId) const {
  return m_impl->getCustomPageSizeMM(pageId);
}

void Settings::setPageSize(const PageId& pageId, const PageSizeMode mode, const QSizeF& customSizeMm) {
  m_impl->setPageSize(pageId, mode, customSizeMm);
}

QSizeF Settings::getTargetPageSizeMM(const PageId& pageId) const {
  return m_impl->getTargetPageSizeMM(pageId);
}

QSizeF Settings::getTargetPageSizeMM(const PageId& pageId,
                                     const QSizeF& hardSizeMm,
                                     const Alignment& alignment) const {
  return m_impl->getTargetPageSizeMM(pageId, hardSizeMm, alignment);
}

bool Settings::hasFrontCover() const {
  return m_impl->hasFrontCover();
}

bool Settings::isParamsNull(const PageId& pageId) const {
  return m_impl->isParamsNull(pageId);
}

const DeviationProvider<PageId>& Settings::deviationProvider() const {
  return m_impl->deviationProvider();
}

std::vector<Guide>& Settings::guides() {
  return m_impl->guides();
}

bool Settings::isShowingMiddleRectEnabled() const {
  return m_impl->isShowingMiddleRectEnabled();
}

void Settings::enableShowingMiddleRect(const bool state) {
  m_impl->enableShowingMiddleRect(state);
}

/*============================== Settings::Item =============================*/

Settings::Item::Item(const PageId& pageId,
                     const Margins& hardMarginsMm,
                     const QRectF& pageRect,
                     const QRectF& contentRect,
                     const QSizeF& contentSizeMm,
                     const Alignment& alignment,
                     const bool autoMargins,
                     const PageSizeMode pageSizeMode,
                     const QSizeF& customPageSizeMm)
    : pageId(pageId),
      hardMarginsMM(hardMarginsMm),
      pageRect(pageRect),
      contentRect(contentRect),
      contentSizeMM(contentSizeMm),
      alignment(alignment),
      autoMargins(autoMargins),
      pageSizeMode(pageSizeMode),
      customPageSizeMM(customPageSizeMm) {}

double Settings::Item::hardWidthMM() const {
  return contentSizeMM.width() + hardMarginsMM.left() + hardMarginsMM.right();
}

double Settings::Item::hardHeightMM() const {
  return contentSizeMM.height() + hardMarginsMM.top() + hardMarginsMM.bottom();
}

double Settings::Item::influenceHardWidthMM() const {
  return alignment.isNull() ? 0.0 : hardWidthMM();
}

double Settings::Item::influenceHardHeightMM() const {
  return alignment.isNull() ? 0.0 : hardHeightMM();
}

/*============================= Settings::Impl ==============================*/

Settings::Impl::Impl(FrontCoverProvider frontCoverProvider)
    : m_items(),
      m_unorderedItems(m_items.get<SequencedTag>()),
      m_descWidthOrder(m_items.get<DescWidthTag>()),
      m_descHeightOrder(m_items.get<DescHeightTag>()),
      m_invalidRect(),
      m_invalidSize(),
      m_defaultHardMarginsMM(Margins(10.0, 5.0, 10.0, 5.0)),
      m_defaultAlignment(Alignment::TOP, Alignment::HCENTER),
      m_autoMarginsDefault(false),
      m_showMiddleRect(true),
      m_frontCoverProvider(std::move(frontCoverProvider)) {
  m_deviationProvider.setComputeValueByKey([this](const PageId& pageId) -> double {
    auto it = m_items.find(pageId);
    if (it != m_items.end()) {
      const Margins& marginsMm = it->hardMarginsMM;
      double horHardMargins = marginsMm.left() + marginsMm.right();
      double vertHardMargins = marginsMm.top() + marginsMm.bottom();
      return std::sqrt(std::pow(horHardMargins, 2) + std::pow(vertHardMargins, 2));
    }
    return NAN;
  });
}

Settings::Impl::~Impl() = default;

void Settings::Impl::clear() {
  const QMutexLocker locker(&m_mutex);
  m_items.clear();
  m_deviationProvider.clear();
  m_frozenAggregateHardSizeMm.reset();
}

void Settings::Impl::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  Container newItems;

  for (const Item& item : m_unorderedItems) {
    const RelinkablePath oldPath(item.pageId.imageId().filePath(), RelinkablePath::File);
    Item newItem(item);
    newItem.pageId.imageId().setFilePath(relinker.substitutionPathFor(oldPath));
    newItems.insert(newItem);
  }

  m_items.swap(newItems);

  m_deviationProvider.clear();
  for (const Item& item : m_unorderedItems) {
    m_deviationProvider.addOrUpdate(item.pageId);
  }
}

void Settings::Impl::removePagesMissingFrom(const PageSequence& pages) {
  const QMutexLocker locker(&m_mutex);

  std::vector<PageId> sortedPages;

  sortedPages.reserve(pages.numPages());
  for (const PageInfo& page : pages) {
    sortedPages.push_back(page.id());
  }
  std::sort(sortedPages.begin(), sortedPages.end());

  UnorderedItems::const_iterator it(m_unorderedItems.begin());
  const UnorderedItems::const_iterator end(m_unorderedItems.end());
  while (it != end) {
    if (std::binary_search(sortedPages.begin(), sortedPages.end(), it->pageId)) {
      ++it;
    } else {
      m_deviationProvider.remove(it->pageId);
      m_unorderedItems.erase(it++);
    }
  }
}

bool Settings::Impl::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  const QMutexLocker locker(&m_mutex);

  for (const PageInfo& pageInfo : pages) {
    if (ignore && (*ignore == pageInfo.id())) {
      continue;
    }
    const Container::iterator it(m_items.find(pageInfo.id()));
    if ((it == m_items.end()) || !it->contentSizeMM.isValid()) {
      return false;
    }
  }
  return true;
}

std::unique_ptr<Params> Settings::Impl::getPageParams(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return nullptr;
  }
  return std::make_unique<Params>(it->hardMarginsMM, it->pageRect, it->contentRect, it->contentSizeMM, it->alignment,
                                  it->autoMargins, it->pageSizeMode, it->customPageSizeMM);
}

void Settings::Impl::setPageParams(const PageId& pageId, const Params& params) {
  const QMutexLocker locker(&m_mutex);

  const Item newItem(pageId, params.hardMarginsMM(), params.pageRect(), params.contentRect(), params.contentSizeMM(),
                     params.alignment(), params.isAutoMarginsEnabled(), params.pageSizeMode(),
                     params.customPageSizeMM());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    m_items.insert(it, newItem);
  } else {
    m_items.replace(it, newItem);
  }

  m_deviationProvider.addOrUpdate(pageId);
}

Params Settings::Impl::updateContentSizeAndGetParams(const PageId& pageId,
                                                     const QRectF& pageRect,
                                                     const QRectF& contentRect,
                                                     const QSizeF& contentSizeMm,
                                                     QSizeF* aggHardSizeBefore,
                                                     QSizeF* aggHardSizeAfter) {
  const QMutexLocker locker(&m_mutex);

  if (aggHardSizeBefore) {
    *aggHardSizeBefore = getAggregateHardSizeMMLocked();
  }

  const Container::iterator it(m_items.find(pageId));
  Container::iterator itemIt(it);
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, pageRect, contentRect, contentSizeMm, m_defaultAlignment,
                    m_autoMarginsDefault);
    itemIt = m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(contentSizeMm, contentRect, pageRect));
  }

  if (aggHardSizeAfter) {
    *aggHardSizeAfter = getAggregateHardSizeMMLocked();
  }

  m_deviationProvider.addOrUpdate(pageId);
  return Params(itemIt->hardMarginsMM, itemIt->pageRect, itemIt->contentRect, itemIt->contentSizeMM, itemIt->alignment,
                itemIt->autoMargins, itemIt->pageSizeMode, itemIt->customPageSizeMM);
}  // Settings::Impl::updateContentSizeAndGetParams

Margins Settings::Impl::getHardMarginsMM(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_defaultHardMarginsMM;
  } else {
    return it->hardMarginsMM;
  }
}

void Settings::Impl::setHardMarginsMM(const PageId& pageId, const Margins& marginsMm) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, marginsMm, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(marginsMm, it->autoMargins));
  }

  m_deviationProvider.addOrUpdate(pageId);
}

Alignment Settings::Impl::getPageAlignment(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_defaultAlignment;
  } else {
    return it->alignment;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setPageAlignment(const PageId& pageId, const Alignment& alignment) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF aggSizeBefore(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, alignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyAlignment(alignment));
  }

  m_deviationProvider.addOrUpdate(pageId);

  const QSizeF aggSizeAfter(getAggregateHardSizeMMLocked());
  if (aggSizeBefore == aggSizeAfter) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF aggSizeBefore(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, contentSizeMm, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(contentSizeMm, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(pageId);

  const QSizeF aggSizeAfter(getAggregateHardSizeMMLocked());
  if (aggSizeBefore == aggSizeAfter) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

void Settings::Impl::invalidateContentSize(const PageId& pageId) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it != m_items.end()) {
    m_items.modify(it, ModifyContentSize(m_invalidSize, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(pageId);
}

QSizeF Settings::Impl::getAggregateHardSizeMM() const {
  const QMutexLocker locker(&m_mutex);
  return getAggregateHardSizeMMLocked();
}

QSizeF Settings::Impl::computeAggregateHardSizeMMLocked() const {
  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  const Item& maxWidthItem = *m_descWidthOrder.begin();
  const Item& maxHeightItem = *m_descHeightOrder.begin();

  const double width = maxWidthItem.influenceHardWidthMM();
  const double height = maxHeightItem.influenceHardHeightMM();
  return QSizeF(width, height);
}

QSizeF Settings::Impl::getAggregateHardSizeMMLocked() const {
  if (m_frozenAggregateHardSizeMm) {
    return *m_frozenAggregateHardSizeMm;
  }
  return computeAggregateHardSizeMMLocked();
}

void Settings::Impl::setAggregateHardSizeFrozen(const bool frozen) {
  const QMutexLocker locker(&m_mutex);
  if (frozen) {
    m_frozenAggregateHardSizeMm = computeAggregateHardSizeMMLocked();
  } else {
    m_frozenAggregateHardSizeMm.reset();
  }
}

bool Settings::Impl::isAggregateHardSizeFrozen() const {
  const QMutexLocker locker(&m_mutex);
  return m_frozenAggregateHardSizeMm.has_value();
}

QSizeF Settings::Impl::getAggregateHardSizeMM(const PageId& pageId,
                                              const QSizeF& hardSizeMm,
                                              const Alignment& alignment) const {
  const QMutexLocker locker(&m_mutex);
  return aggregateHardSizeWithOverrideMMLocked(pageId, hardSizeMm, alignment);
}

QSizeF Settings::Impl::aggregateHardSizeWithOverrideMMLocked(const PageId& pageId,
                                                             const QSizeF& hardSizeMm,
                                                             const Alignment& alignment) const {
  if (m_frozenAggregateHardSizeMm) {
    return *m_frozenAggregateHardSizeMm;
  }

  if (alignment.isNull()) {
    return computeAggregateHardSizeMMLocked();
  }

  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  double width = 0.0;

  {
    DescWidthOrder::iterator it(m_descWidthOrder.begin());
    if (it->pageId != pageId) {
      width = it->influenceHardWidthMM();
    } else {
      ++it;
      if (it == m_descWidthOrder.end()) {
        width = hardSizeMm.width();
      } else {
        width = std::max(hardSizeMm.width(), qreal(it->influenceHardWidthMM()));
      }
    }
  }

  double height = 0.0;

  {
    DescHeightOrder::iterator it(m_descHeightOrder.begin());
    if (it->pageId != pageId) {
      height = it->influenceHardHeightMM();
    } else {
      ++it;
      if (it == m_descHeightOrder.end()) {
        height = hardSizeMm.height();
      } else {
        height = std::max(hardSizeMm.height(), qreal(it->influenceHardHeightMM()));
      }
    }
  }
  return QSizeF(width, height);
}  // Settings::Impl::getAggregateHardSizeMM

bool Settings::Impl::isPageAutoMarginsEnabled(const PageId& pageId) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return m_autoMarginsDefault;
  } else {
    return it->autoMargins;
  }
}

void Settings::Impl::setPageAutoMarginsEnabled(const PageId& pageId, const bool state) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    state);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(it->hardMarginsMM, state));
  }
}

PageSizeMode Settings::Impl::getPageSizeMode(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);
  const Container::const_iterator it(m_items.find(pageId));
  return (it == m_items.end()) ? PageSizeMode::AUTO : it->pageSizeMode;
}

QSizeF Settings::Impl::getCustomPageSizeMM(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);
  const Container::const_iterator it(m_items.find(pageId));
  return (it == m_items.end()) ? QSizeF() : it->customPageSizeMM;
}

void Settings::Impl::setPageSize(const PageId& pageId,
                                 const PageSizeMode mode,
                                 const QSizeF& customSizeMm) {
  const QMutexLocker locker(&m_mutex);
  const Container::iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    const Item item(pageId, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    m_autoMarginsDefault, mode, customSizeMm);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyPageSize(mode, customSizeMm));
  }
}

QSizeF Settings::Impl::getTargetPageSizeMM(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);
  const Container::const_iterator it(m_items.find(pageId));
  if (it == m_items.end()) {
    return getAggregateHardSizeMMLocked();
  }
  return targetPageSizeMMLocked(pageId, QSizeF(it->hardWidthMM(), it->hardHeightMM()), it->alignment);
}

QSizeF Settings::Impl::getTargetPageSizeMM(const PageId& pageId,
                                           const QSizeF& hardSizeMm,
                                           const Alignment& alignment) const {
  const QMutexLocker locker(&m_mutex);
  return targetPageSizeMMLocked(pageId, hardSizeMm, alignment);
}

QSizeF Settings::Impl::targetPageSizeMMLocked(const PageId& pageId,
                                              const QSizeF& hardSizeMm,
                                              const Alignment& alignment) const {
  const Container::const_iterator current(m_items.find(pageId));
  const PageSizeMode mode = (current == m_items.end()) ? PageSizeMode::AUTO : current->pageSizeMode;

  if (mode == PageSizeMode::CUSTOM) {
    return boundedTargetSize(current->customPageSizeMM, hardSizeMm);
  }

  if ((mode == PageSizeMode::FRONT_COVER) && m_frontCoverProvider) {
    const PageId frontCoverId = m_frontCoverProvider();
    const Container::const_iterator frontCover(m_items.find(frontCoverId));
    if (!frontCoverId.isNull() && (frontCover != m_items.end())) {
      QSizeF frontCoverHardSize(frontCover->hardWidthMM(), frontCover->hardHeightMM());
      if (frontCoverId == pageId) {
        frontCoverHardSize = hardSizeMm;
      }
      QSizeF requestedSize = frontCoverHardSize;
      if ((frontCover->pageSizeMode == PageSizeMode::CUSTOM) && !frontCover->customPageSizeMM.isEmpty()) {
        requestedSize = boundedTargetSize(frontCover->customPageSizeMM, frontCoverHardSize);
      }
      return boundedTargetSize(requestedSize, hardSizeMm);
    }
  }

  return aggregateHardSizeWithOverrideMMLocked(pageId, hardSizeMm, alignment);
}

QSizeF Settings::Impl::boundedTargetSize(const QSizeF& requestedSizeMm, const QSizeF& hardSizeMm) {
  if (!requestedSizeMm.isValid() || requestedSizeMm.isEmpty()) {
    return hardSizeMm;
  }
  return QSizeF(std::max(requestedSizeMm.width(), hardSizeMm.width()),
                std::max(requestedSizeMm.height(), hardSizeMm.height()));
}

bool Settings::Impl::hasFrontCover() const {
  return m_frontCoverProvider && !m_frontCoverProvider().isNull();
}

bool Settings::Impl::isParamsNull(const PageId& pageId) const {
  const QMutexLocker locker(&m_mutex);
  return (m_items.find(pageId) == m_items.end());
}

const DeviationProvider<PageId>& Settings::Impl::deviationProvider() const {
  return m_deviationProvider;
}

std::vector<Guide>& Settings::Impl::guides() {
  return m_guides;
}

bool Settings::Impl::isShowingMiddleRectEnabled() const {
  return m_showMiddleRect;
}

void Settings::Impl::enableShowingMiddleRect(const bool state) {
  m_showMiddleRect = state;
}
}  // namespace page_layout
