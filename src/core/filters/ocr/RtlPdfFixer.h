// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_RTLPDFFIXER_H_
#define SCANTAILOR_OCR_RTLPDFFIXER_H_

#include <QCoreApplication>

class QString;

namespace ocr {
class RtlPdfFixer {
  Q_DECLARE_TR_FUNCTIONS(ocr::RtlPdfFixer)
 public:
  /**
   * Replaces Tesseract's mirrored invisible text with a logical-order text layer.
   * Page images and their compression are preserved unchanged.
   */
  static bool rewrite(const QString& inputPdfPath,
                      const QString& tsvPath,
                      const QString& outputPdfPath,
                      QString* errorMessage = nullptr);
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_RTLPDFFIXER_H_
