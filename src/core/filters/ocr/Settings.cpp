// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"

#include <QDomDocument>
#include <QDomElement>

namespace ocr {
namespace {
PdfCompression compressionFromInt(const int value) {
  switch (value) {
    case static_cast<int>(PdfCompression::HIGH_QUALITY):
      return PdfCompression::HIGH_QUALITY;
    case static_cast<int>(PdfCompression::SMALL):
      return PdfCompression::SMALL;
    case static_cast<int>(PdfCompression::BALANCED):
    default:
      return PdfCompression::BALANCED;
  }
}
}  // namespace

Settings::Settings() : m_arabicEnabled(true), m_englishEnabled(true), m_compression(PdfCompression::BALANCED) {}

Settings::Settings(const QDomElement& element) : Settings() {
  if (element.isNull()) {
    return;
  }
  m_arabicEnabled = element.attribute(QStringLiteral("arabic"), QStringLiteral("1")).toInt() != 0;
  m_englishEnabled = element.attribute(QStringLiteral("english"), QStringLiteral("1")).toInt() != 0;
  m_compression
      = compressionFromInt(element.attribute(QStringLiteral("compression"), QStringLiteral("1")).toInt());
}

int Settings::jpegQuality() const {
  switch (m_compression) {
    case PdfCompression::HIGH_QUALITY:
      return 95;
    case PdfCompression::SMALL:
      return 60;
    case PdfCompression::BALANCED:
    default:
      return 82;
  }
}

double Settings::estimatedSizeRatio() const {
  switch (m_compression) {
    case PdfCompression::HIGH_QUALITY:
      return 0.18;
    case PdfCompression::SMALL:
      return 0.045;
    case PdfCompression::BALANCED:
    default:
      return 0.08;
  }
}

QDomElement Settings::toXml(QDomDocument& document, const QString& name) const {
  QDomElement element(document.createElement(name));
  element.setAttribute(QStringLiteral("arabic"), m_arabicEnabled ? 1 : 0);
  element.setAttribute(QStringLiteral("english"), m_englishEnabled ? 1 : 0);
  element.setAttribute(QStringLiteral("compression"), static_cast<int>(m_compression));
  return element;
}
}  // namespace ocr
