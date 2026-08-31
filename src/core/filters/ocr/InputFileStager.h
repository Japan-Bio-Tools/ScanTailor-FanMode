// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OCR_INPUTFILESTAGER_H_
#define SCANTAILOR_OCR_INPUTFILESTAGER_H_

#include <QCoreApplication>
#include <QStringList>

class QString;

namespace ocr {
/** Prepares OCR inputs under predictable ASCII-only filenames. */
class InputFileStager {
  Q_DECLARE_TR_FUNCTIONS(ocr::InputFileStager)
 public:
  static bool stage(const QStringList& sourceFiles,
                    const QString& destinationDirectory,
                    QStringList* stagedFiles,
                    QString* errorMessage = nullptr);
};
}  // namespace ocr

#endif  // SCANTAILOR_OCR_INPUTFILESTAGER_H_
