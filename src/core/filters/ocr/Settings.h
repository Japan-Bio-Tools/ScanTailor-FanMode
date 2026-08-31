// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_SETTINGS_H_
#define SCANTAILOR_OCR_SETTINGS_H_

class QDomDocument;
class QDomElement;
class QString;

namespace ocr {
enum class PdfCompression { HIGH_QUALITY = 0, BALANCED = 1, SMALL = 2 };

class Settings {
 public:
  Settings();

  explicit Settings(const QDomElement& element);

  bool arabicEnabled() const { return m_arabicEnabled; }

  void setArabicEnabled(bool enabled) { m_arabicEnabled = enabled; }

  bool englishEnabled() const { return m_englishEnabled; }

  void setEnglishEnabled(bool enabled) { m_englishEnabled = enabled; }

  PdfCompression compression() const { return m_compression; }

  void setCompression(PdfCompression compression) { m_compression = compression; }

  int jpegQuality() const;

  double estimatedSizeRatio() const;

  QDomElement toXml(QDomDocument& document, const QString& name) const;

 private:
  bool m_arabicEnabled;
  bool m_englishEnabled;
  PdfCompression m_compression;
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_SETTINGS_H_
