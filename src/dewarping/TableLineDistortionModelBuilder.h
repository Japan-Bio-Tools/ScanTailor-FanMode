// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_TABLELINEDISTORTIONMODELBUILDER_H_
#define SCANTAILOR_DEWARPING_TABLELINEDISTORTIONMODELBUILDER_H_

#include <QRect>

#include "DistortionModel.h"

namespace imageproc {
class GrayImage;
}

namespace dewarping {
/**
 * Builds a cylindrical dewarping model from long rules in tables, forms and
 * technical drawings.  Unlike TextLineTracer this detector deliberately
 * ignores short text strokes and only accepts a coherent family of rules.
 */
class TableLineDistortionModelBuilder {
 public:
  struct Result {
    DistortionModel model;
    int candidateLineCount = 0;
    int acceptedLineCount = 0;
    bool reliable = false;
  };

  static Result build(const imageproc::GrayImage& image, const QRect& contentRect);
};
}  // namespace dewarping

#endif  // SCANTAILOR_DEWARPING_TABLELINEDISTORTIONMODELBUILDER_H_
