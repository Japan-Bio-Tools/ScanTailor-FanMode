// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_IMAGEPDFWRITER_H_
#define SCANTAILOR_OCR_IMAGEPDFWRITER_H_

#include <QCoreApplication>
#include <QStringList>
#include <functional>

class QString;

namespace ocr {
class ImagePdfWriter {
  Q_DECLARE_TR_FUNCTIONS(ImagePdfWriter)

 public:
  using ProgressCallback = std::function<bool(int completedPages, int totalPages)>;

  /** Creates an image-only PDF. Returning false with an empty error means cancellation. */
  static bool write(const QStringList& imageFiles,
                    const QString& outputPdfPath,
                    int jpegQuality,
                    const ProgressCallback& progressCallback = ProgressCallback(),
                    QString* errorMessage = nullptr);
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_IMAGEPDFWRITER_H_
