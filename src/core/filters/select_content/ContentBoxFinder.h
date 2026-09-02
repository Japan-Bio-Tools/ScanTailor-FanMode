// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_CONTENTBOXFINDER_H_
#define SCANTAILOR_SELECT_CONTENT_CONTENTBOXFINDER_H_

#include <BinaryThreshold.h>

class TaskStatus;
class DebugImages;
class FilterData;
class QImage;
class QRect;
class QRectF;

namespace imageproc {
class BinaryImage;
class ConnComp;

class SEDM;
}  // namespace imageproc

namespace select_content {
struct ContentDetectionOptions {
  bool accurate = false;
  bool includeHeaderTitle = true;
  bool includePageNumber = true;
};

class ContentBoxFinder {
 public:
  static QRectF findContentBox(const TaskStatus& status,
                               const FilterData& data,
                               const QRectF& pageRect,
                               const ContentDetectionOptions& options = ContentDetectionOptions(),
                               DebugImages* dbg = nullptr);

  /**
   * Expands a standard content rectangle with plausible text lines from the
   * page's upper and lower edge bands. Coordinates are image pixels.
   * Exposed for deterministic regression tests of Accurate mode.
   */
  static QRect expandContentBoxForEdgeText(const imageproc::BinaryImage& content,
                                           const QRect& pageRect,
                                           const QRect& standardContentRect,
                                           bool includeHeaderTitle,
                                           bool includePageNumber);

 private:
  class Garbage;

  static void segmentGarbage(const imageproc::BinaryImage& garbage,
                             imageproc::BinaryImage& horGarbage,
                             imageproc::BinaryImage& vertGarbage,
                             DebugImages* dbg);

  static void trimContentBlocksInPlace(const imageproc::BinaryImage& content, imageproc::BinaryImage& contentBlocks);

  static void inPlaceRemoveAreasTouchingBorders(imageproc::BinaryImage& contentBlocks, DebugImages* dbg);

  static imageproc::BinaryImage estimateTextMask(const imageproc::BinaryImage& content,
                                                 const imageproc::BinaryImage& contentBlocks,
                                                 DebugImages* dbg);

  static void filterShadows(const TaskStatus& status, imageproc::BinaryImage& shadows, DebugImages* dbg);

  static QRect trimLeft(const imageproc::BinaryImage& content,
                        const imageproc::BinaryImage& contentBlocks,
                        const imageproc::BinaryImage& textMask,
                        const QRect& area,
                        Garbage& garbage,
                        DebugImages* dbg);

  static QRect trimRight(const imageproc::BinaryImage& content,
                         const imageproc::BinaryImage& contentBlocks,
                         const imageproc::BinaryImage& textMask,
                         const QRect& area,
                         Garbage& garbage,
                         DebugImages* dbg);

  static QRect trimTop(const imageproc::BinaryImage& content,
                       const imageproc::BinaryImage& contentBlocks,
                       const imageproc::BinaryImage& textMask,
                       const QRect& area,
                       Garbage& garbage,
                       DebugImages* dbg);

  static QRect trimBottom(const imageproc::BinaryImage& content,
                          const imageproc::BinaryImage& contentBlocks,
                          const imageproc::BinaryImage& textMask,
                          const QRect& area,
                          Garbage& garbage,
                          DebugImages* dbg);

  static QRect trim(const imageproc::BinaryImage& content,
                    const imageproc::BinaryImage& contentBlocks,
                    const imageproc::BinaryImage& textMask,
                    const QRect& area,
                    const QRect& newArea,
                    const QRect& removedArea,
                    Garbage& garbage,
                    bool& canRetryGrouped,
                    DebugImages* dbg);
};
}  // namespace select_content
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_CONTENTBOXFINDER_H_
