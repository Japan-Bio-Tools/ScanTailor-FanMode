// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImagePdfWriter.h"

#include <core/ImageLoader.h>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>
#include <qpdf/QPDFWriter.hh>

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QtGlobal>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace ocr {
namespace {
struct EncodedImage {
  QByteArray bytes;
  QByteArray filter;
  QByteArray colorSpace;
  int bitsPerComponent = 8;
};

double imageDpi(const int dotsPerMeter) {
  const double dpi = dotsPerMeter > 0 ? dotsPerMeter * 0.0254 : 0.0;
  return (dpi >= 30.0 && dpi <= 2400.0) ? dpi : 300.0;
}

bool encodeImage(const QImage& source, const int jpegQuality, EncodedImage* encoded) {
  if (source.depth() == 1) {
    const QImage grayscale(source.convertToFormat(QImage::Format_Grayscale8));
    QByteArray raw;
    raw.reserve(grayscale.width() * grayscale.height());
    for (int y = 0; y < grayscale.height(); ++y) {
      raw.append(reinterpret_cast<const char*>(grayscale.constScanLine(y)), grayscale.width());
    }
    encoded->bytes = qCompress(raw, 9);
    if (encoded->bytes.size() <= 4) {
      return false;
    }
    // qCompress prepends a four-byte uncompressed-size field. PDF Flate
    // streams contain only the zlib payload.
    encoded->bytes.remove(0, 4);
    encoded->filter = QByteArrayLiteral("/FlateDecode");
    encoded->colorSpace = QByteArrayLiteral("/DeviceGray");
    return true;
  }

  QImage image;
  if (source.isGrayscale()) {
    image = source.convertToFormat(QImage::Format_Grayscale8);
    encoded->colorSpace = QByteArrayLiteral("/DeviceGray");
  } else {
    image = QImage(source.size(), QImage::Format_RGB888);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.drawImage(QPoint(0, 0), source);
    painter.end();
    encoded->colorSpace = QByteArrayLiteral("/DeviceRGB");
  }

  QBuffer buffer(&encoded->bytes);
  if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "JPEG", qBound(1, jpegQuality, 100))) {
    return false;
  }
  encoded->filter = QByteArrayLiteral("/DCTDecode");
  return true;
}

QPDFObjectHandle numberArray(const std::vector<double>& values) {
  std::vector<QPDFObjectHandle> objects;
  objects.reserve(values.size());
  for (const double value : values) {
    objects.push_back(QPDFObjectHandle::newReal(value, 4, true));
  }
  return QPDFObjectHandle::newArray(objects);
}
}  // namespace

bool ImagePdfWriter::write(const QStringList& imageFiles,
                           const QString& outputPdfPath,
                           const int jpegQuality,
                           const ProgressCallback& progressCallback,
                           QString* errorMessage) {
  auto fail = [&](const QString& message) {
    if (errorMessage) {
      *errorMessage = message;
    }
    return false;
  };
  if (errorMessage) {
    errorMessage->clear();
  }
  if (imageFiles.isEmpty()) {
    return fail(ImagePdfWriter::tr("There are no page images to export."));
  }

  try {
    QPDF pdf;
    pdf.emptyPDF();
    const int totalPages = imageFiles.size();

    for (int pageIndex = 0; pageIndex < totalPages; ++pageIndex) {
      if (progressCallback && !progressCallback(pageIndex, totalPages)) {
        return false;
      }

      const QString& imagePath = imageFiles.at(pageIndex);
      // ScanTailor output is TIFF.  The Windows package intentionally relies
      // on the application's libtiff reader rather than Qt's optional TIFF
      // image-format plugin, so QImage(imagePath) is not sufficient here.
      const QImage image(ImageLoader::load(imagePath));
      if (image.isNull()) {
        return fail(ImagePdfWriter::tr("Unable to read page image:\n%1").arg(QFileInfo(imagePath).absoluteFilePath()));
      }

      EncodedImage encoded;
      if (!encodeImage(image, jpegQuality, &encoded)) {
        return fail(
            ImagePdfWriter::tr("Unable to compress page image:\n%1").arg(QFileInfo(imagePath).absoluteFilePath()));
      }

      const double pageWidth = image.width() * 72.0 / imageDpi(image.dotsPerMeterX());
      const double pageHeight = image.height() * 72.0 / imageDpi(image.dotsPerMeterY());

      const std::string imageData(encoded.bytes.constData(), static_cast<size_t>(encoded.bytes.size()));
      QPDFObjectHandle imageObject(pdf.newStream(imageData));
      QPDFObjectHandle imageDictionary(imageObject.getDict());
      imageDictionary.replaceKey("/Type", QPDFObjectHandle::newName("/XObject"));
      imageDictionary.replaceKey("/Subtype", QPDFObjectHandle::newName("/Image"));
      imageDictionary.replaceKey("/Width", QPDFObjectHandle::newInteger(image.width()));
      imageDictionary.replaceKey("/Height", QPDFObjectHandle::newInteger(image.height()));
      imageDictionary.replaceKey("/ColorSpace", QPDFObjectHandle::newName(encoded.colorSpace.constData()));
      imageDictionary.replaceKey("/BitsPerComponent", QPDFObjectHandle::newInteger(encoded.bitsPerComponent));
      imageDictionary.replaceKey("/Filter", QPDFObjectHandle::newName(encoded.filter.constData()));

      std::ostringstream content;
      content.imbue(std::locale::classic());
      content << std::fixed << std::setprecision(4) << "q\n"
              << pageWidth << " 0 0 " << pageHeight << " 0 0 cm\n/Im0 Do\nQ\n";
      QPDFObjectHandle contents(pdf.newStream(content.str()));

      QPDFObjectHandle xObjects(QPDFObjectHandle::newDictionary());
      xObjects.replaceKey("/Im0", imageObject);
      QPDFObjectHandle resources(QPDFObjectHandle::newDictionary());
      resources.replaceKey("/XObject", xObjects);

      QPDFObjectHandle page(QPDFObjectHandle::newDictionary());
      page.replaceKey("/Type", QPDFObjectHandle::newName("/Page"));
      page.replaceKey("/MediaBox", numberArray({0.0, 0.0, pageWidth, pageHeight}));
      page.replaceKey("/Resources", resources);
      page.replaceKey("/Contents", contents);
      pdf.addPage(pdf.makeIndirectObject(page), false);
    }

    if (progressCallback && !progressCallback(totalPages, totalPages)) {
      return false;
    }
    QPDFWriter writer(pdf, outputPdfPath.toUtf8().constData());
    writer.setObjectStreamMode(qpdf_o_generate);
    writer.write();
  } catch (const std::exception& error) {
    return fail(
        ImagePdfWriter::tr("Unable to create the image-only PDF:\n%1").arg(QString::fromLocal8Bit(error.what())));
  }
  return true;
}
}  // namespace ocr
