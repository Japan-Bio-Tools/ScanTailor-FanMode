// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "InputFileStager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace ocr {
namespace {
bool stageOneFile(const QString& sourcePath, const QString& destinationPath) {
#ifdef Q_OS_WIN
  // Both the project and the temporary directory are normally on the same
  // volume.  A hard link is instant and consumes no additional image space.
  if (CreateHardLinkW(reinterpret_cast<const wchar_t*>(destinationPath.utf16()),
                      reinterpret_cast<const wchar_t*>(sourcePath.utf16()), nullptr)) {
    return true;
  }
#else
  if (QFile::link(sourcePath, destinationPath)) {
    return true;
  }
#endif
  return QFile::copy(sourcePath, destinationPath);
}
}  // namespace

bool InputFileStager::stage(const QStringList& sourceFiles,
                            const QString& destinationDirectory,
                            QStringList* stagedFiles,
                            QString* errorMessage) {
  if (!stagedFiles) {
    if (errorMessage) {
      *errorMessage = tr("Unable to prepare the Output pages for OCR.");
    }
    return false;
  }
  stagedFiles->clear();

  QDir destination(destinationDirectory);
  if (!destination.mkpath(QStringLiteral("."))) {
    if (errorMessage) {
      *errorMessage = tr("Unable to create the temporary OCR input folder.");
    }
    return false;
  }

  for (int index = 0; index < sourceFiles.size(); ++index) {
    const QFileInfo source(sourceFiles.at(index));
    if (!source.isFile()) {
      if (errorMessage) {
        *errorMessage = tr("An Output page disappeared before OCR could start:\n%1\n\n"
                           "Run Output for this page, then export again.")
                            .arg(QDir::toNativeSeparators(source.absoluteFilePath()));
      }
      return false;
    }

    const QString stagedName = QStringLiteral("ocr-page-%1.tif").arg(index + 1, 6, 10, QChar('0'));
    const QString stagedPath = destination.filePath(stagedName);
    if (!stageOneFile(source.absoluteFilePath(), stagedPath)) {
      if (errorMessage) {
        *errorMessage = tr("Unable to prepare this Output page for OCR:\n%1\n\n"
                           "Check available disk space and try again.")
                            .arg(QDir::toNativeSeparators(source.absoluteFilePath()));
      }
      return false;
    }
    stagedFiles->append(QFileInfo(stagedPath).absoluteFilePath());
  }
  return true;
}
}  // namespace ocr
