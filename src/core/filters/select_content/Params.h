// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
#define SCANTAILOR_SELECT_CONTENT_PARAMS_H_

#include <AutoManualMode.h>

#include <QRectF>
#include <QSizeF>
#include <cmath>

#include "Dependencies.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace select_content {
class Params {
 public:
  // Member-wise copying is OK.

  explicit Params(const Dependencies& deps);

  Params(const QRectF& contentRect,
         const QSizeF& sizeMm,
         const QRectF& pageRect,
         const Dependencies& deps,
         AutoManualMode contentDetectionMode,
         AutoManualMode pageDetectionMode,
         bool fineTuneCorners,
         bool accurateContentDetection = true,
         bool includeHeaderTitle = true,
         bool includePageNumber = true);

  explicit Params(const QDomElement& filterEl);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  ~Params();

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Dependencies& dependencies() const;

  AutoManualMode contentDetectionMode() const;

  AutoManualMode pageDetectionMode() const;

  bool isFineTuningEnabled() const;

  bool isAccurateContentDetectionEnabled() const;

  bool isHeaderTitleIncluded() const;

  bool isPageNumberIncluded() const;

  void setContentDetectionMode(AutoManualMode mode);

  void setPageDetectionMode(AutoManualMode mode);

  void setContentRect(const QRectF& rect);

  void setPageRect(const QRectF& rect);

  void setContentSizeMM(const QSizeF& size);

  void setDependencies(const Dependencies& deps);

  void setFineTuneCornersEnabled(bool fineTuneCorners);

  void setAccurateContentDetectionEnabled(bool enabled);

  void setHeaderTitleIncluded(bool included);

  void setPageNumberIncluded(bool included);

 private:
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Dependencies m_deps;
  AutoManualMode m_contentDetectionMode;
  AutoManualMode m_pageDetectionMode;
  bool m_fineTuneCorners;
  bool m_accurateContentDetection;
  bool m_includeHeaderTitle;
  bool m_includePageNumber;
};


inline const QRectF& Params::contentRect() const {
  return m_contentRect;
}

inline const QRectF& Params::pageRect() const {
  return m_pageRect;
}

inline const QSizeF& Params::contentSizeMM() const {
  return m_contentSizeMM;
}

inline const Dependencies& Params::dependencies() const {
  return m_deps;
}

inline AutoManualMode Params::contentDetectionMode() const {
  return m_contentDetectionMode;
}

inline AutoManualMode Params::pageDetectionMode() const {
  return m_pageDetectionMode;
}

inline bool Params::isFineTuningEnabled() const {
  return m_fineTuneCorners;
}

inline bool Params::isAccurateContentDetectionEnabled() const {
  return m_accurateContentDetection;
}

inline bool Params::isHeaderTitleIncluded() const {
  return m_includeHeaderTitle;
}

inline bool Params::isPageNumberIncluded() const {
  return m_includePageNumber;
}

inline void Params::setContentDetectionMode(const AutoManualMode mode) {
  m_contentDetectionMode = mode;
}

inline void Params::setPageDetectionMode(const AutoManualMode mode) {
  m_pageDetectionMode = mode;
}

inline void Params::setContentRect(const QRectF& rect) {
  m_contentRect = rect;
}

inline void Params::setPageRect(const QRectF& rect) {
  m_pageRect = rect;
}

inline void Params::setContentSizeMM(const QSizeF& size) {
  m_contentSizeMM = size;
}

inline void Params::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

inline void Params::setFineTuneCornersEnabled(bool fineTuneCorners) {
  m_fineTuneCorners = fineTuneCorners;
}

inline void Params::setAccurateContentDetectionEnabled(bool enabled) {
  m_accurateContentDetection = enabled;
}

inline void Params::setHeaderTitleIncluded(bool included) {
  m_includeHeaderTitle = included;
}

inline void Params::setPageNumberIncluded(bool included) {
  m_includePageNumber = included;
}
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
