// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_DEWARPINGVIEW_H_
#define SCANTAILOR_OUTPUT_DEWARPINGVIEW_H_

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QImage>
#include <QTimer>
#include <QTransform>
#include <array>
#include <vector>

#include "DepthPerception.h"
#include "DewarpingOptions.h"
#include "DistortionModel.h"
#include "DragHandler.h"
#include "DraggablePoint.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "InteractionHandler.h"
#include "InteractiveXSpline.h"
#include "ObjectDragHandler.h"
#include "PageId.h"
#include "Settings.h"
#include "ZoomHandler.h"

class QShortcut;

namespace output {
class DewarpingView : public ImageViewBase, protected InteractionHandler {
  Q_OBJECT
 public:
  DewarpingView(const QImage& image,
                const ImagePixmapUnion& downscaledImage,
                const QTransform& sourceToVirt,
                const QPolygonF& virtDisplayArea,
                const QRectF& virtContentRect,
                const PageId& pageId,
                DewarpingOptions dewarpingOptions,
                const dewarping::DistortionModel& distortionModel,
                const DepthPerception& depthPerception);

  ~DewarpingView() override;

 signals:

  void distortionModelChanged(const dewarping::DistortionModel& model);

 public slots:

  void depthPerceptionChanged(double val);

  void livePreviewChanged(bool enabled);

  void simpleManualModeChanged(bool enabled);

  void resetToNormalGrid();

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private:
  static void initNewSpline(XSpline& spline,
                            const QPointF& p1,
                            const QPointF& p2,
                            const DewarpingOptions* dewarpingOptions = nullptr);

  static void fitSpline(XSpline& spline, const std::vector<QPointF>& polyline);

  void paintXSpline(QPainter& painter, const InteractionState& interaction, const InteractiveXSpline& ispline);

  void paintSimpleControls(QPainter& painter, const InteractionState& interaction);

  void curveModified(int curveIdx);

  void dragFinished();

  void scheduleLivePreview(int delayMs = 120);

  void rebuildLivePreview();

  void paintInPlacePreview(QPainter& painter) const;

  QPointF simpleAnchorPosition(int curveIdx, int anchorIdx) const;

  QPointF simpleHandlePosition(int curveIdx, int anchorIdx) const;

  QPointF simpleBorderPosition(int borderIdx, int curveIdx) const;

  QPointF simpleBorderHandlePosition(int borderIdx, int curveIdx) const;

  void simpleHandleMoveRequest(int curveIdx,
                               int anchorIdx,
                               const QPointF& widgetPos,
                               Qt::KeyboardModifiers modifiers);

  void simpleHandleDragFinished(int curveIdx, int anchorIdx);

  void simpleBorderMoveRequest(int borderIdx,
                               int curveIdx,
                               const QPointF& widgetPos,
                               Qt::KeyboardModifiers modifiers);

  void simpleBorderDragFinished();

  void moveSimpleAnchor(int curveIdx, int anchorIdx, const QPointF& sourcePos);

  void moveSimpleBorderPoint(int borderIdx, int curveIdx, const QPointF& sourcePos);

  QPointF snapToNearbyTextLine(const QPointF& sourcePos) const;

  void setSimpleHandlersLinked(bool linked);

  QPointF sourceToWidget(const QPointF& pt) const;

  QPointF widgetToSource(const QPointF& pt) const;

  QPolygonF virtMarginArea(int marginIdx) const;

  PageId m_pageId;
  QPolygonF m_virtDisplayArea;
  QPolygonF m_sourceContentRect;
  DewarpingOptions m_dewarpingOptions;
  dewarping::DistortionModel m_distortionModel;
  DepthPerception m_depthPerception;
  InteractiveXSpline m_topSpline;
  InteractiveXSpline m_bottomSpline;
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
  QShortcut* m_removeControlPointShortcut;
  QImage m_previewSourceImage;
  QImage m_livePreviewImage;
  QTransform m_sourceToPreview;
  QTimer m_livePreviewTimer;
  bool m_livePreviewEnabled;
  bool m_simpleManualEnabled;

  struct SimpleHandle {
    DraggablePoint point;
    ObjectDragHandler handler;
  };
  std::array<std::array<SimpleHandle, 3>, 2> m_simpleHandles;
  std::array<std::array<SimpleHandle, 2>, 2> m_simpleBorderHandles;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_DEWARPINGVIEW_H_
