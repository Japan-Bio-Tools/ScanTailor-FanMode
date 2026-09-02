// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RtlPdfFixer.h"

#include <qpdf/Buffer.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFPageDocumentHelper.hh>
#include <qpdf/QPDFPageObjectHelper.hh>
#include <qpdf/QPDFWriter.hh>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ocr {
namespace {
struct Word {
  int left = 0;
  int top = 0;
  int width = 0;
  int height = 0;
  QString text;
};

struct Page {
  int width = 0;
  int height = 0;
  std::vector<Word> words;
};

bool parseTsv(const QString& path, std::vector<Page>* pages, QString* errorMessage) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    if (errorMessage) {
      *errorMessage = RtlPdfFixer::tr("Unable to read Tesseract's logical OCR data.");
    }
    return false;
  }

  const QString contents = QString::fromUtf8(file.readAll());
  const QStringList lines = contents.split(QChar('\n'), Qt::SkipEmptyParts);
  for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex) {
    QString line = lines.at(lineIndex);
    if (line.endsWith(QChar('\r'))) {
      line.chop(1);
    }
    const QStringList fields = line.split(QChar('\t'), Qt::KeepEmptyParts);
    if (fields.size() < 12) {
      continue;
    }

    bool levelOk = false;
    bool pageOk = false;
    const int level = fields.at(0).toInt(&levelOk);
    const int pageNumber = fields.at(1).toInt(&pageOk);
    if (!levelOk || !pageOk || (pageNumber < 1)) {
      continue;
    }
    if (pages->size() < static_cast<size_t>(pageNumber)) {
      pages->resize(pageNumber);
    }

    Page& page = pages->at(pageNumber - 1);
    if (level == 1) {
      page.width = fields.at(8).toInt();
      page.height = fields.at(9).toInt();
      continue;
    }
    if (level != 5) {
      continue;
    }

    Word word;
    word.left = fields.at(6).toInt();
    word.top = fields.at(7).toInt();
    word.width = fields.at(8).toInt();
    word.height = fields.at(9).toInt();
    word.text = fields.mid(11).join(QChar('\t'));
    if (!word.text.isEmpty() && (word.width > 0) && (word.height > 0)) {
      page.words.emplace_back(std::move(word));
    }
  }

  if (pages->empty()) {
    if (errorMessage) {
      *errorMessage = RtlPdfFixer::tr("Tesseract did not return any page layout data.");
    }
    return false;
  }
  return true;
}

std::string utf16Hex(const QString& text, const bool byteOrderMark) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::hex << std::nouppercase << std::setfill('0');
  if (byteOrderMark) {
    stream << "feff";
  }
  for (const QChar character : text) {
    stream << std::setw(4) << character.unicode();
  }
  return stream.str();
}

bool isRtlText(const QString& text) {
  for (const QChar character : text) {
    if ((character.direction() == QChar::DirR) || (character.direction() == QChar::DirAL)) {
      return true;
    }
    if (character.direction() == QChar::DirL) {
      return false;
    }
  }
  return false;
}

QString reversedCodePoints(const QString& text) {
  auto codePoints = text.toUcs4();
  std::reverse(codePoints.begin(), codePoints.end());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return QString::fromUcs4(reinterpret_cast<const char32_t*>(codePoints.constData()), codePoints.size());
#else
  return QString::fromUcs4(codePoints.constData(), codePoints.size());
#endif
}

int stripTesseractText(std::string* content) {
  constexpr const char* beginMarker = "BT\n3 Tr";
  constexpr const char* endMarker = "ET\n";
  int removed = 0;
  std::string::size_type position = 0;
  while ((position = content->find(beginMarker, position)) != std::string::npos) {
    const std::string::size_type end = content->find(endMarker, position);
    if (end == std::string::npos) {
      break;
    }
    content->erase(position, end + std::char_traits<char>::length(endMarker) - position);
    ++removed;
  }
  return removed;
}

std::string logicalTextLayer(const Page& page,
                             const double pageLeft,
                             const double pageBottom,
                             const double pageWidth,
                             const double pageHeight) {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::fixed << std::setprecision(3);

  const double scaleX = pageWidth / std::max(1, page.width);
  const double scaleY = pageHeight / std::max(1, page.height);
  for (const Word& word : page.words) {
    const QString searchableText = word.text + QChar(' ');
    const int codePointCount = std::max(1, static_cast<int>(searchableText.toUcs4().size()));
    const double x = pageLeft + word.left * scaleX;
    const double y = pageBottom + pageHeight - (word.top + word.height) * scaleY;
    const double wordWidth = std::max(0.1, word.width * scaleX);
    const double fontSize = std::max(1.0, word.height * scaleY);
    const double horizontalStretch = std::clamp(200.0 * wordWidth / (fontSize * codePointCount), 1.0, 2000.0);
    // PDF extractors apply the Unicode bidi algorithm after reading glyphs in
    // geometric order.  Put RTL code points into visual order on a positive text
    // matrix so that this bidi pass restores their logical order.  This follows
    // OCRmyPDF's current multilingual invisible-text renderer.  LTR words and
    // numbers remain unchanged, which keeps mixed Arabic / English lines intact.
    const QString encodedText = isRtlText(word.text) ? reversedCodePoints(searchableText) : searchableText;
    const std::string textHex = utf16Hex(encodedText, false);

    stream << "BT\n3 Tr\n/f-0-0 " << fontSize << " Tf\n"
           << "1 0 0 1 " << x << ' ' << y << " Tm\n"
           << horizontalStretch << " Tz\n"
           << "[ <" << textHex << "> ] TJ\nET\n";
  }
  return stream.str();
}

bool fail(const QString& message, QString* errorMessage) {
  if (errorMessage) {
    *errorMessage = message;
  }
  return false;
}
}  // namespace

bool RtlPdfFixer::rewrite(const QString& inputPdfPath,
                          const QString& tsvPath,
                          const QString& outputPdfPath,
                          QString* errorMessage) {
  std::vector<Page> pageData;
  if (!parseTsv(tsvPath, &pageData, errorMessage)) {
    return false;
  }

  try {
    QPDF pdf;
    pdf.processFile(inputPdfPath.toUtf8().constData());
    QPDFPageDocumentHelper document(pdf);
    std::vector<QPDFPageObjectHelper> pdfPages = document.getAllPages();
    if (pdfPages.size() != pageData.size()) {
      return fail(tr("The OCR page count does not match the generated PDF."), errorMessage);
    }

    int removedTextBlocks = 0;
    for (size_t pageIndex = 0; pageIndex < pdfPages.size(); ++pageIndex) {
      QPDFPageObjectHelper& pdfPage = pdfPages[pageIndex];
      for (QPDFObjectHandle& contentStream : pdfPage.getPageContents()) {
        const std::shared_ptr<Buffer> buffer = contentStream.getStreamData(qpdf_dl_generalized);
        std::string content(reinterpret_cast<const char*>(buffer->getBuffer()), buffer->getSize());
        const int removed = stripTesseractText(&content);
        if (removed > 0) {
          removedTextBlocks += removed;
          contentStream.replaceStreamData(content, QPDFObjectHandle::newNull(), QPDFObjectHandle::newNull());
        }
      }

      std::vector<QPDFObjectHandle> mediaBox = pdfPage.getMediaBox().getArrayAsVector();
      if (mediaBox.size() != 4) {
        return fail(tr("A generated PDF page has an invalid page box."), errorMessage);
      }
      const double left = mediaBox[0].getNumericValue();
      const double bottom = mediaBox[1].getNumericValue();
      const double width = mediaBox[2].getNumericValue() - left;
      const double height = mediaBox[3].getNumericValue() - bottom;
      if ((width <= 0) || (height <= 0) || (pageData[pageIndex].width <= 0) || (pageData[pageIndex].height <= 0)) {
        return fail(tr("A generated PDF page has invalid dimensions."), errorMessage);
      }
      pdfPage.addPageContents(pdf.newStream(logicalTextLayer(pageData[pageIndex], left, bottom, width, height)), false);
    }

    if (removedTextBlocks == 0) {
      return fail(tr("The original Tesseract text layer could not be located."), errorMessage);
    }

    QPDFWriter writer(pdf, outputPdfPath.toUtf8().constData());
    writer.setObjectStreamMode(qpdf_o_generate);
    writer.setCompressStreams(true);
    writer.write();
  } catch (const std::exception& exception) {
    return fail(
        tr("Unable to create the corrected searchable text layer: %1").arg(QString::fromLocal8Bit(exception.what())),
        errorMessage);
  }
  return true;
}
}  // namespace ocr
