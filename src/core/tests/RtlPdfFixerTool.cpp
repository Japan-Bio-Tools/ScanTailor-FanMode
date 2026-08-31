// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <filters/ocr/RtlPdfFixer.h>

#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[]) {
  QCoreApplication application(argc, argv);
  QTextStream errorOutput(stderr);
  if (application.arguments().size() != 4) {
    errorOutput << "Usage: rtl_pdf_fixer_tool input.pdf input.tsv output.pdf\n";
    return 2;
  }

  QString errorMessage;
  const QStringList arguments = application.arguments();
  if (!ocr::RtlPdfFixer::rewrite(arguments.at(1), arguments.at(2), arguments.at(3), &errorMessage)) {
    errorOutput << errorMessage << '\n';
    return 1;
  }
  return 0;
}
