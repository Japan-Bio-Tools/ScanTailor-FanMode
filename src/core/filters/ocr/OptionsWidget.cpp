// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "AbstractRelinker.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "RtlPdfFixer.h"
#include "Settings.h"
#include "filters/output/Filter.h"

namespace ocr {
OptionsWidget::OptionsWidget(std::shared_ptr<Settings> settings,
                             std::shared_ptr<ProjectPages> pages,
                             std::weak_ptr<output::Filter> outputFilter)
    : m_settings(std::move(settings)), m_pages(std::move(pages)), m_outputFilter(std::move(outputFilter)) {
  setupUi(this);

  compressionCombo->addItem(tr("High quality (largest)"), static_cast<int>(PdfCompression::HIGH_QUALITY));
  compressionCombo->addItem(tr("Balanced (recommended)"), static_cast<int>(PdfCompression::BALANCED));
  compressionCombo->addItem(tr("Small file"), static_cast<int>(PdfCompression::SMALL));

  connect(arabicCheckBox, &QCheckBox::toggled, this, &OptionsWidget::languagesChanged);
  connect(englishCheckBox, &QCheckBox::toggled, this, &OptionsWidget::languagesChanged);
  connect(compressionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &OptionsWidget::compressionChanged);
  connect(exportPdfButton, &QPushButton::clicked, this, &OptionsWidget::exportPdf);

  syncFromSettings();
}

void OptionsWidget::preUpdateUI(const PageId& pageId) {
  m_pageId = pageId;
  updateEstimate();
}

void OptionsWidget::postUpdateUI() {
  updateEstimate();
}

void OptionsWidget::syncFromSettings() {
  const QSignalBlocker blockArabic(arabicCheckBox);
  const QSignalBlocker blockEnglish(englishCheckBox);
  const QSignalBlocker blockCompression(compressionCombo);
  arabicCheckBox->setChecked(m_settings->arabicEnabled());
  englishCheckBox->setChecked(m_settings->englishEnabled());
  const int index = compressionCombo->findData(static_cast<int>(m_settings->compression()));
  compressionCombo->setCurrentIndex(index >= 0 ? index : 1);
  updateEstimate();
}

void OptionsWidget::setExportContext(const OutputFileNameGenerator& outputFileNameGenerator,
                                     const QString& projectFilePath) {
  m_outputFileNameGenerator = outputFileNameGenerator;
  m_projectFilePath = projectFilePath;
  updateEstimate();
}

void OptionsWidget::performRelinking(const AbstractRelinker& relinker) {
  m_outputFileNameGenerator.performRelinking(relinker);
  updateEstimate();
}

void OptionsWidget::languagesChanged() {
  m_settings->setArabicEnabled(arabicCheckBox->isChecked());
  m_settings->setEnglishEnabled(englishCheckBox->isChecked());
  updateEstimate();
}

void OptionsWidget::compressionChanged(const int index) {
  if (index >= 0) {
    m_settings->setCompression(static_cast<PdfCompression>(compressionCombo->itemData(index).toInt()));
  }
  updateEstimate();
}

std::vector<PageId> OptionsWidget::orderedPageIds() const {
  std::vector<PageId> pages;
  if (!m_pages) {
    return pages;
  }

  const PageSequence sequence = m_pages->toPageSequence(PAGE_VIEW);
  const std::shared_ptr<output::Filter> outputFilter = m_outputFilter.lock();
  const PageId frontCover = outputFilter ? outputFilter->frontCoverPageId() : PageId();
  const PageId backCover = outputFilter ? outputFilter->backCoverPageId() : PageId();

  auto sequenceContains = [&](const PageId& pageId) {
    return !pageId.isNull() && sequence.pageNo(pageId) >= 0;
  };
  if (sequenceContains(frontCover)) {
    pages.push_back(frontCover);
  }
  for (const PageInfo& pageInfo : sequence) {
    if ((pageInfo.id() != frontCover) && (pageInfo.id() != backCover)) {
      pages.push_back(pageInfo.id());
    }
  }
  if (sequenceContains(backCover) && (backCover != frontCover)) {
    pages.push_back(backCover);
  }
  return pages;
}

QStringList OptionsWidget::outputFiles(int* missingCount, qint64* totalBytes) const {
  int missing = 0;
  qint64 bytes = 0;
  QStringList files;
  for (const PageId& pageId : orderedPageIds()) {
    const QString filePath = m_outputFileNameGenerator.filePathFor(pageId);
    const QFileInfo fileInfo(filePath);
    if (fileInfo.isFile()) {
      files.append(fileInfo.absoluteFilePath());
      bytes += fileInfo.size();
    } else {
      ++missing;
    }
  }
  if (missingCount) {
    *missingCount = missing;
  }
  if (totalBytes) {
    *totalBytes = bytes;
  }
  return files;
}

void OptionsWidget::updateEstimate() {
  const bool hasLanguage = arabicCheckBox->isChecked() || englishCheckBox->isChecked();
  int missing = 0;
  qint64 sourceBytes = 0;
  const QStringList files = outputFiles(&missing, &sourceBytes);
  exportPdfButton->setEnabled(hasLanguage && !files.isEmpty() && (missing == 0));

  if (!hasLanguage) {
    estimateLabel->setText(tr("Select at least one OCR language."));
    return;
  }
  if (files.isEmpty()) {
    estimateLabel->setText(tr("No final TIFF pages are available. Run Output first."));
    return;
  }
  if (missing > 0) {
    estimateLabel->setText(tr("%1 page(s) still need Output processing.").arg(missing));
    return;
  }

  const qint64 estimatedBytes
      = qMax<qint64>(files.size() * 4096, qRound64(sourceBytes * m_settings->estimatedSizeRatio()));
  estimateLabel->setText(tr("Estimated PDF size: about %1 (%2 pages)")
                             .arg(formatBytes(estimatedBytes))
                             .arg(files.size()));
}

QString OptionsWidget::selectedLanguages() const {
  QStringList languages;
  const bool rtl = m_pages && (m_pages->layoutDirection() == Qt::RightToLeft);
  if (rtl) {
    if (arabicCheckBox->isChecked()) {
      languages.append(QStringLiteral("ara"));
    }
    if (englishCheckBox->isChecked()) {
      languages.append(QStringLiteral("eng"));
    }
  } else {
    if (englishCheckBox->isChecked()) {
      languages.append(QStringLiteral("eng"));
    }
    if (arabicCheckBox->isChecked()) {
      languages.append(QStringLiteral("ara"));
    }
  }
  return languages.join(QChar('+'));
}

QString OptionsWidget::suggestedPdfPath() const {
  QSettings settings;
  QString directory = settings.value(QStringLiteral("ocr/lastExportDirectory")).toString();
  if (directory.isEmpty() || !QDir(directory).exists()) {
    directory = m_outputFileNameGenerator.outDir();
  }
  if (directory.isEmpty() || !QDir(directory).exists()) {
    directory = QDir::homePath();
  }

  QString baseName = QFileInfo(m_projectFilePath).completeBaseName().trimmed();
  if (baseName.isEmpty()) {
    baseName = QStringLiteral("ScanTailor-FanMode");
  }
  return QDir(directory).filePath(baseName + QStringLiteral("-searchable.pdf"));
}

QString OptionsWidget::formatBytes(const qint64 bytes) {
  constexpr qint64 kib = 1024;
  constexpr qint64 mib = 1024 * kib;
  constexpr qint64 gib = 1024 * mib;
  if (bytes >= gib) {
    return tr("%1 GB").arg(QString::number(static_cast<double>(bytes) / gib, 'f', 2));
  }
  if (bytes >= mib) {
    return tr("%1 MB").arg(QString::number(static_cast<double>(bytes) / mib, 'f', 1));
  }
  if (bytes >= kib) {
    return tr("%1 KB").arg(QString::number(static_cast<double>(bytes) / kib, 'f', 0));
  }
  return tr("%1 bytes").arg(bytes);
}

QString OptionsWidget::findBundledTool(const QString& toolName) {
#ifdef Q_OS_WIN
  const QString executableName = toolName + QStringLiteral(".exe");
#else
  const QString executableName = toolName;
#endif
  const QString bundledPath = QDir(QCoreApplication::applicationDirPath()).filePath(executableName);
  if (QFileInfo::exists(bundledPath)) {
    return bundledPath;
  }
  return QStandardPaths::findExecutable(toolName);
}

void OptionsWidget::exportPdf() {
  int missing = 0;
  const QStringList files = outputFiles(&missing);
  if (missing > 0 || files.isEmpty()) {
    QMessageBox::warning(this, tr("OCR & PDF"),
                         tr("All pages must have final Output TIFF files before PDF export.\n"
                            "Run batch processing in Output, then try again."));
    updateEstimate();
    return;
  }

  const QString languages = selectedLanguages();
  if (languages.isEmpty()) {
    QMessageBox::warning(this, tr("OCR & PDF"), tr("Select Arabic, English, or both."));
    return;
  }

  QString targetPath = QFileDialog::getSaveFileName(this, tr("Export Searchable PDF"), suggestedPdfPath(),
                                                     tr("PDF documents (*.pdf)"));
  if (targetPath.isEmpty()) {
    return;
  }
  if (!targetPath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
    targetPath += QStringLiteral(".pdf");
  }

  const QString tesseractPath = findBundledTool(QStringLiteral("tesseract"));
  const QString tessdataPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("tessdata"));
  if (tesseractPath.isEmpty() || !QDir(tessdataPath).exists()) {
    QMessageBox::warning(this, tr("OCR Engine Not Found"),
                         tr("The bundled OCR engine or its language data is missing.\n"
                            "Please reinstall ScanTailor FanMode."));
    return;
  }
  for (const QString& language : languages.split(QChar('+'))) {
    if (!QFileInfo::exists(QDir(tessdataPath).filePath(language + QStringLiteral(".traineddata")))) {
      QMessageBox::warning(this, tr("OCR Language Not Found"),
                           tr("The selected OCR language data is missing: %1").arg(language));
      return;
    }
  }

  QTemporaryDir temporaryDirectory(
      QDir(QDir::tempPath()).filePath(QStringLiteral("ScanTailor-FanMode-OCR-XXXXXX")));
  if (!temporaryDirectory.isValid()) {
    QMessageBox::warning(this, tr("OCR & PDF"), tr("Unable to create a temporary OCR folder."));
    return;
  }

  const QString imageListPath = QDir(temporaryDirectory.path()).filePath(QStringLiteral("pages.txt"));
  QFile imageListFile(imageListPath);
  if (!imageListFile.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, tr("OCR & PDF"), tr("Unable to prepare the page list for OCR."));
    return;
  }
  QByteArray imageListData;
  for (const QString& filePath : files) {
    imageListData.append(QDir::toNativeSeparators(filePath).toUtf8());
    imageListData.append('\n');
  }
  if (imageListFile.write(imageListData) != imageListData.size()) {
    QMessageBox::warning(this, tr("OCR & PDF"), tr("Unable to write the page list for OCR."));
    return;
  }
  imageListFile.close();

  const QString outputBase = QDir(temporaryDirectory.path()).filePath(QStringLiteral("searchable-book"));
  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.setProgram(tesseractPath);
  process.setArguments({QStringLiteral("--tessdata-dir"),
                        tessdataPath,
                        imageListPath,
                        outputBase,
                        QStringLiteral("-l"),
                        languages,
                        QStringLiteral("--oem"),
                        QStringLiteral("1"),
                        QStringLiteral("--psm"),
                        QStringLiteral("3"),
                        QStringLiteral("-c"),
                        QStringLiteral("jpg_quality=%1").arg(m_settings->jpegQuality()),
                        QStringLiteral("-c"),
                        QStringLiteral("preserve_interword_spaces=1"),
                        QStringLiteral("pdf"),
                        QStringLiteral("tsv")});

  const int pageCount = static_cast<int>(files.size());
  QProgressDialog progress(tr("Recognizing page 1 of %1...").arg(pageCount), tr("Cancel"), 0, pageCount, this);
  progress.setWindowTitle(tr("OCR & PDF Export"));
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  progress.setValue(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);

  const QRegularExpression pageLine(QStringLiteral("\\bPage\\s+(\\d+)"), QRegularExpression::CaseInsensitiveOption);
  QString processOutput;
  auto readOutput = [&]() {
    processOutput += QString::fromUtf8(process.readAllStandardOutput());
    if (processOutput.size() > 16000) {
      processOutput = processOutput.right(16000);
    }
    QRegularExpressionMatchIterator matches = pageLine.globalMatch(processOutput);
    int lastPage = 0;
    while (matches.hasNext()) {
      lastPage = matches.next().captured(1).toInt() + 1;
    }
    if (lastPage > 0) {
      lastPage = qBound(1, lastPage, pageCount);
      progress.setValue(lastPage);
      progress.setLabelText(tr("Recognizing page %1 of %2...").arg(lastPage).arg(pageCount));
    }
  };
  connect(&process, &QProcess::readyReadStandardOutput, this, readOutput);

  bool canceled = false;
  connect(&progress, &QProgressDialog::canceled, this, [&]() {
    canceled = true;
    if (process.state() != QProcess::NotRunning) {
      process.kill();
    }
  });

  QEventLoop waitLoop;
  connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &waitLoop, &QEventLoop::quit);
  process.start();
  if (!process.waitForStarted(5000)) {
    progress.hide();
    QMessageBox::warning(this, tr("OCR & PDF Export Failed"),
                         tr("Unable to start the OCR engine:\n%1").arg(process.errorString()));
    return;
  }

  progress.show();
  if (process.state() != QProcess::NotRunning) {
    waitLoop.exec();
  }
  readOutput();
  progress.hide();

  if (canceled) {
    return;
  }

  const QString temporaryPdf = outputBase + QStringLiteral(".pdf");
  const QString temporaryTsv = outputBase + QStringLiteral(".tsv");
  if ((process.exitStatus() != QProcess::NormalExit) || (process.exitCode() != 0)
      || !QFileInfo::exists(temporaryPdf) || !QFileInfo::exists(temporaryTsv)) {
    QString details = processOutput.trimmed();
    if (details.size() > 1500) {
      details = details.right(1500);
    }
    QMessageBox::warning(this, tr("OCR & PDF Export Failed"),
                         details.isEmpty() ? tr("The searchable PDF could not be created.")
                                           : tr("The searchable PDF could not be created.\n\n%1").arg(details));
    return;
  }

  progress.setLabelText(tr("Correcting Arabic and bilingual text order..."));
  progress.show();
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  const QString correctedPdf = outputBase + QStringLiteral("-rtl-safe.pdf");
  QString correctionError;
  if (!RtlPdfFixer::rewrite(temporaryPdf, temporaryTsv, correctedPdf, &correctionError)) {
    progress.hide();
    QMessageBox::warning(this, tr("OCR & PDF Export Failed"), correctionError);
    return;
  }
  progress.hide();

  QFile sourcePdf(correctedPdf);
  QSaveFile destinationPdf(targetPath);
  if (!sourcePdf.open(QIODevice::ReadOnly) || !destinationPdf.open(QIODevice::WriteOnly)) {
    QMessageBox::warning(this, tr("OCR & PDF Export Failed"), tr("Unable to write the selected PDF file."));
    return;
  }
  while (!sourcePdf.atEnd()) {
    const QByteArray chunk = sourcePdf.read(1024 * 1024);
    if (chunk.isEmpty() && sourcePdf.error() != QFileDevice::NoError) {
      destinationPdf.cancelWriting();
      QMessageBox::warning(this, tr("OCR & PDF Export Failed"), tr("Unable to read the temporary PDF file."));
      return;
    }
    if (destinationPdf.write(chunk) != chunk.size()) {
      destinationPdf.cancelWriting();
      QMessageBox::warning(this, tr("OCR & PDF Export Failed"), tr("Unable to write the selected PDF file."));
      return;
    }
  }
  if (!destinationPdf.commit()) {
    QMessageBox::warning(this, tr("OCR & PDF Export Failed"), tr("Unable to finish writing the PDF file."));
    return;
  }

  QSettings appSettings;
  appSettings.setValue(QStringLiteral("ocr/lastExportDirectory"), QFileInfo(targetPath).absolutePath());
  const QString actualSize = formatBytes(QFileInfo(targetPath).size());
  lastExportLabel->setText(tr("Last export: %1").arg(actualSize));
  QMessageBox::information(this, tr("OCR & PDF Export Complete"),
                           tr("Created a searchable %1-page PDF.\n\nActual size: %2\n%3")
                               .arg(files.size())
                               .arg(actualSize)
                               .arg(QDir::toNativeSeparators(targetPath)));
}
}  // namespace ocr
