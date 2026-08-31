// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Params.h"

#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

namespace page_layout {
namespace {
PageSizeMode pageSizeModeFromXml(const QDomElement& el) {
  const QString value = el.attribute("pageSizeMode", "auto");
  if (value == "custom") {
    return PageSizeMode::CUSTOM;
  }
  if (value == "front-cover") {
    return PageSizeMode::FRONT_COVER;
  }
  return PageSizeMode::AUTO;
}

const char* pageSizeModeToXml(const PageSizeMode mode) {
  switch (mode) {
    case PageSizeMode::CUSTOM:
      return "custom";
    case PageSizeMode::FRONT_COVER:
      return "front-cover";
    case PageSizeMode::AUTO:
    default:
      return "auto";
  }
}
}  // namespace

Params::Params(const Margins& hardMarginsMm,
               const QRectF& pageRect,
               const QRectF& contentRect,
               const QSizeF& contentSizeMm,
               const Alignment& alignment,
               const bool autoMargins,
               const PageSizeMode pageSizeMode,
               const QSizeF& customPageSizeMm)
    : m_hardMarginsMM(hardMarginsMm),
      m_contentRect(contentRect),
      m_pageRect(pageRect),
      m_contentSizeMM(contentSizeMm),
      m_alignment(alignment),
      m_autoMargins(autoMargins),
      m_pageSizeMode(pageSizeMode),
      m_customPageSizeMM(customPageSizeMm) {}

Params::Params(const QDomElement& el)
    : m_hardMarginsMM(el.namedItem("hardMarginsMM").toElement()),
      m_contentRect(XmlUnmarshaller::rectF(el.namedItem("contentRect").toElement())),
      m_pageRect(XmlUnmarshaller::rectF(el.namedItem("pageRect").toElement())),
      m_contentSizeMM(XmlUnmarshaller::sizeF(el.namedItem("contentSizeMM").toElement())),
      m_alignment(el.namedItem("alignment").toElement()),
      m_autoMargins(el.attribute("autoMargins") == "1"),
      m_pageSizeMode(pageSizeModeFromXml(el)),
      m_customPageSizeMM(XmlUnmarshaller::sizeF(el.namedItem("customPageSizeMM").toElement())) {}

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(m_hardMarginsMM.toXml(doc, "hardMarginsMM"));
  el.appendChild(marshaller.rectF(m_pageRect, "pageRect"));
  el.appendChild(marshaller.rectF(m_contentRect, "contentRect"));
  el.appendChild(marshaller.sizeF(m_contentSizeMM, "contentSizeMM"));
  el.appendChild(marshaller.sizeF(m_customPageSizeMM, "customPageSizeMM"));
  el.appendChild(m_alignment.toXml(doc, "alignment"));
  el.setAttribute("autoMargins", m_autoMargins ? "1" : "0");
  el.setAttribute("pageSizeMode", QString::fromLatin1(pageSizeModeToXml(m_pageSizeMode)));
  return el;
}
}  // namespace page_layout
