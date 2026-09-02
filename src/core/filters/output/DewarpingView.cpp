// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DewarpingView.h"

#include <CylindricalSurfaceDewarper.h>
#include <RasterDewarper.h>

#include <QDebug>
#include <QFontMetrics>
#include <QPainter>
#include <QShortcut>
#include <boost/bind/bind.hpp>
#include <algorithm>
#include <cmath>
#include <exception>

#include "ImagePresentation.h"
#include "ToLineProjector.h"
#include "spfit/ConstraintSet.h"
#include "spfit/LinearForceBalancer.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/SplineFitter.h"

namespace output {
DewarpingView::DewarpingView(const QImage& image,
                             const ImagePixmapUnion& downscaledImage,
                             const QTransform& imageToVirt,
                             const QPolygonF& virtDisplayArea,
                             const QRectF& virtContentRect,
                             const PageId& pageId,
                             DewarpingOptions dewarpingOptions,
                             const dewarping::DistortionModel& distortionModel,
                             const DepthPerception& depthPerception)
    : ImageViewBase(image, downscaledImage, ImagePresentation(imageToVirt, virtDisplayArea)),
      m_pageId(pageId),
      m_virtDisplayArea(virtDisplayArea),
      m_dewarpingOptions(dewarpingOptions),
      m_distortionModel(distortionModel),
      m_depthPerception(depthPerception),
      m_topSpline(*this),
      m_bottomSpline(*this),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_livePreviewEnabled(false),
      m_simpleManualEnabled(false) {
  setMouseTracking(true);

  m_sourceContentRect = virtualToImage().map(virtContentRect);

  constexpr int maxPreviewSide = 1200;
  if (std::max(image.width(), image.height()) > maxPreviewSide) {
    m_previewSourceImage = image.scaled(maxPreviewSide, maxPreviewSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  } else {
    m_previewSourceImage = image;
  }
  m_sourceToPreview.scale(static_cast<double>(m_previewSourceImage.width()) / image.width(),
                          static_cast<double>(m_previewSourceImage.height()) / image.height());
  m_livePreviewTimer.setSingleShot(true);
  QObject::connect(&m_livePreviewTimer, &QTimer::timeout, this, &DewarpingView::rebuildLivePreview);

  XSpline topSpline(m_distortionModel.topCurve().xspline());
  XSpline bottomSpline(m_distortionModel.bottomCurve().xspline());
  if (topSpline.numControlPoints() < 2) {
    const std::vector<QPointF>& polyline = m_distortionModel.topCurve().polyline();

    XSpline newTopSpline;
    if (polyline.size() < 2) {
      initNewSpline(newTopSpline, m_sourceContentRect[0], m_sourceContentRect[1], &dewarpingOptions);
    } else {
      initNewSpline(newTopSpline, polyline.front(), polyline.back(), &dewarpingOptions);
      fitSpline(newTopSpline, polyline);
    }

    topSpline.swap(newTopSpline);
  }
  if (bottomSpline.numControlPoints() < 2) {
    const std::vector<QPointF>& polyline = m_distortionModel.bottomCurve().polyline();

    XSpline newBottomSpline;
    if (polyline.size() < 2) {
      initNewSpline(newBottomSpline, m_sourceContentRect[3], m_sourceContentRect[2], &dewarpingOptions);
    } else {
      initNewSpline(newBottomSpline, polyline.front(), polyline.back(), &dewarpingOptions);
      fitSpline(newBottomSpline, polyline);
    }

    bottomSpline.swap(newBottomSpline);
  }

  m_topSpline.setSpline(topSpline);
  m_bottomSpline.setSpline(bottomSpline);

  InteractiveXSpline* splines[2] = {&m_topSpline, &m_bottomSpline};
  int curveIdx = -1;
  for (InteractiveXSpline* spline : splines) {
    ++curveIdx;
    spline->setModifiedCallback(boost::bind(&DewarpingView::curveModified, this, curveIdx));
    spline->setDragFinishedCallback(boost::bind(&DewarpingView::dragFinished, this));
    spline->setStorageTransform(boost::bind(&DewarpingView::sourceToWidget, this, boost::placeholders::_1),
                                boost::bind(&DewarpingView::widgetToSource, this, boost::placeholders::_1));
    makeLastFollower(*spline);
  }

  for (int curveIdx = 0; curveIdx < 2; ++curveIdx) {
    for (int anchorIdx = 0; anchorIdx < 3; ++anchorIdx) {
      SimpleHandle& handle = m_simpleHandles[curveIdx][anchorIdx];
      handle.point.setPositionCallback(
          boost::bind(&DewarpingView::simpleHandlePosition, this, curveIdx, anchorIdx));
      handle.point.setMoveRequestCallback(boost::bind(&DewarpingView::simpleHandleMoveRequest, this, curveIdx,
                                                       anchorIdx, boost::placeholders::_1, boost::placeholders::_2));
      handle.point.setDragFinishedCallback(
          boost::bind(&DewarpingView::simpleHandleDragFinished, this, curveIdx, anchorIdx));
      handle.point.setHitRadius(13.0);
      handle.point.setProximityPriority(10);
      handle.handler.setObject(&handle.point);
      handle.handler.setKeyboardModifiers(
          {Qt::NoModifier, Qt::ShiftModifier, Qt::ControlModifier, Qt::ShiftModifier | Qt::ControlModifier});
      handle.handler.setProximityStatusTip(
          tr("Drag onto the printed text line. The point snaps to nearby dark text when released."));
    }
  }

  for (int borderIdx = 0; borderIdx < 2; ++borderIdx) {
    for (int curveIdx = 0; curveIdx < 2; ++curveIdx) {
      SimpleHandle& handle = m_simpleBorderHandles[borderIdx][curveIdx];
      handle.point.setPositionCallback(
          boost::bind(&DewarpingView::simpleBorderHandlePosition, this, borderIdx, curveIdx));
      handle.point.setMoveRequestCallback(boost::bind(&DewarpingView::simpleBorderMoveRequest, this, borderIdx,
                                                       curveIdx, boost::placeholders::_1, boost::placeholders::_2));
      handle.point.setDragFinishedCallback(boost::bind(&DewarpingView::simpleBorderDragFinished, this));
      handle.point.setHitRadius(13.0);
      handle.point.setProximityPriority(11);
      handle.handler.setObject(&handle.point);
      handle.handler.setKeyboardModifiers(
          {Qt::NoModifier, Qt::ShiftModifier, Qt::ControlModifier, Qt::ShiftModifier | Qt::ControlModifier});
      handle.handler.setProximityStatusTip(
          tr("Drag this blue point to the page border. Each corner moves independently for slanted sides."));
    }
  }

  m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
  m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  m_removeControlPointShortcut = new QShortcut(Qt::Key_D, this);
  QObject::connect(m_removeControlPointShortcut, &QShortcut::activated, [this]() {
    if (m_simpleManualEnabled) {
      return;
    }
    const bool topChanged = m_topSpline.removeControlPointUnderMouse();
    const bool bottomChanged = m_bottomSpline.removeControlPointUnderMouse();
    if (topChanged) {
      curveModified(0);
    }
    if (bottomChanged) {
      curveModified(1);
    }
    if (topChanged || bottomChanged) {
      dragFinished();
    }
  });
}

DewarpingView::~DewarpingView() = default;

void DewarpingView::initNewSpline(XSpline& spline,
                                  const QPointF& p1,
                                  const QPointF& p2,
                                  const DewarpingOptions* dewarpingOptions) {
  const QLineF line(p1, p2);
  spline.appendControlPoint(line.p1(), 0);
  if (dewarpingOptions
      && (((*dewarpingOptions).dewarpingMode() == AUTO) || ((*dewarpingOptions).dewarpingMode() == AUTO_BOOK)
          || ((*dewarpingOptions).dewarpingMode() == AUTO_TABLE))) {
    spline.appendControlPoint(line.pointAt(1.0 / 4.0), 1);
    spline.appendControlPoint(line.pointAt(2.0 / 4.0), 1);
    spline.appendControlPoint(line.pointAt(3.0 / 4.0), 1);
  }
  spline.appendControlPoint(line.p2(), 0);
}

void DewarpingView::fitSpline(XSpline& spline, const std::vector<QPointF>& polyline) {
  using namespace spfit;

  SplineFitter fitter(&spline);
  const PolylineModelShape modelShape(polyline);

  ConstraintSet constraints(&spline);
  constraints.constrainSplinePoint(0.0, polyline.front());
  constraints.constrainSplinePoint(1.0, polyline.back());
  fitter.setConstraints(constraints);

  FittableSpline::SamplingParams samplingParams;
  samplingParams.maxDistBetweenSamples = 10;
  fitter.setSamplingParams(samplingParams);

  int iterationsRemaining = 20;
  LinearForceBalancer balancer(0.8);
  balancer.setTargetRatio(0.1);
  balancer.setIterationsToTarget(iterationsRemaining - 1);

  for (; iterationsRemaining > 0; --iterationsRemaining, balancer.nextIteration()) {
    fitter.addAttractionForces(modelShape);
    fitter.addInternalForce(spline.controlPointsAttractionForce());

    double internalForceWeight = balancer.calcInternalForceWeight(fitter.internalForce(), fitter.externalForce());
    const OptimizationResult res(fitter.optimize(internalForceWeight));
    if (dewarping::Curve::splineHasLoops(spline)) {
      fitter.undoLastStep();
      break;
    }

    if (res.improvementPercentage() < 0.5) {
      break;
    }
  }
}  // DewarpingView::fitSpline

void DewarpingView::depthPerceptionChanged(double val) {
  m_depthPerception.setValue(val);
  scheduleLivePreview();
  update();
}

void DewarpingView::livePreviewChanged(const bool enabled) {
  m_livePreviewEnabled = enabled;
  if (enabled) {
    scheduleLivePreview(0);
  } else {
    m_livePreviewTimer.stop();
    m_livePreviewImage = QImage();
    update();
  }
}

void DewarpingView::simpleManualModeChanged(const bool enabled) {
  if (m_simpleManualEnabled == enabled) {
    return;
  }
  m_simpleManualEnabled = enabled;
  setSimpleHandlersLinked(enabled);
  update();
}

void DewarpingView::resetToNormalGrid() {
  if (m_sourceContentRect.size() < 4) {
    return;
  }

  XSpline topSpline;
  XSpline bottomSpline;
  initNewSpline(topSpline, m_sourceContentRect[0], m_sourceContentRect[1]);
  initNewSpline(bottomSpline, m_sourceContentRect[3], m_sourceContentRect[2]);
  m_topSpline.setSpline(topSpline);
  m_bottomSpline.setSpline(bottomSpline);
  m_distortionModel.setTopCurve(dewarping::Curve(topSpline));
  m_distortionModel.setBottomCurve(dewarping::Curve(bottomSpline));
  m_dewarpingOptions.setDewarpingMode(MANUAL);
  m_dewarpingOptions.setPostDeskewAngle(0.0);
  scheduleLivePreview(0);
  update();
  emit distortionModelChanged(m_distortionModel);
}

void DewarpingView::scheduleLivePreview(const int delayMs) {
  if (m_livePreviewEnabled) {
    m_livePreviewTimer.start(std::max(0, delayMs));
  }
}

void DewarpingView::rebuildLivePreview() {
  if (!m_livePreviewEnabled || !m_distortionModel.isValid() || m_previewSourceImage.isNull()) {
    m_livePreviewImage = QImage();
    update();
    return;
  }

  try {
    std::vector<QPointF> top(m_distortionModel.topCurve().polyline());
    std::vector<QPointF> bottom(m_distortionModel.bottomCurve().polyline());
    for (QPointF& point : top) {
      point = m_sourceToPreview.map(point);
    }
    for (QPointF& point : bottom) {
      point = m_sourceToPreview.map(point);
    }

    const dewarping::CylindricalSurfaceDewarper dewarper(top, bottom, m_depthPerception.value());
    const QRectF previewContentRect = m_sourceToPreview.map(m_sourceContentRect).boundingRect();
    const QRectF modelDomain
        = m_distortionModel.modelDomain(dewarper, m_sourceToPreview, previewContentRect);
    m_livePreviewImage = dewarping::RasterDewarper::dewarp(
        m_previewSourceImage, m_previewSourceImage.size(), dewarper, modelDomain, Qt::white);
  } catch (const std::exception& error) {
    qWarning() << "Live dewarping preview failed:" << error.what();
    m_livePreviewImage = QImage();
  }
  update();
}

void DewarpingView::paintInPlacePreview(QPainter& painter) const {
  if (!m_livePreviewEnabled || m_livePreviewImage.isNull()) {
    return;
  }

  painter.save();
  painter.setWorldTransform(QTransform());
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  const QTransform previewToSource(m_sourceToPreview.inverted());
  const double width = m_livePreviewImage.width();
  const double height = m_livePreviewImage.height();
  QPolygonF previewQuad;
  previewQuad << QPointF(0, 0) << QPointF(width, 0) << QPointF(width, height) << QPointF(0, height);
  QPolygonF widgetQuad;
  for (const QPointF& previewPoint : previewQuad) {
    widgetQuad << sourceToWidget(previewToSource.map(previewPoint));
  }

  QTransform previewToWidget;
  if (QTransform::quadToQuad(previewQuad, widgetQuad, previewToWidget)) {
    painter.setWorldTransform(previewToWidget);
    painter.drawImage(QPointF(0, 0), m_livePreviewImage);
  }
  painter.restore();
}

void DewarpingView::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setRenderHint(QPainter::Antialiasing);

  // The corrected image replaces the original in the same central page area.
  // Controls are painted afterwards so editing remains possible without a
  // separate preview panel obscuring the page.
  paintInPlacePreview(painter);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0xff, 0xff, 0xff, 150));  // Translucent white.
  painter.drawPolygon(virtMarginArea(0));           // Left margin.
  painter.drawPolygon(virtMarginArea(1));           // Right margin.
  painter.setWorldTransform(imageToVirtual() * painter.worldTransform());
  painter.setBrush(Qt::NoBrush);

  QPen gridPen;
  gridPen.setColor(Qt::blue);
  gridPen.setCosmetic(true);
  gridPen.setWidthF(1.2);

  painter.setPen(gridPen);
  painter.setBrush(Qt::NoBrush);

  const int numVertGridLines = 30;
  const int numHorGridLines = 30;

  bool validModel = m_distortionModel.isValid();

  if (!m_simpleManualEnabled && validModel) {
    try {
      std::vector<QVector<QPointF>> curves(numHorGridLines);

      dewarping::CylindricalSurfaceDewarper dewarper(m_distortionModel.topCurve().polyline(),
                                                     m_distortionModel.bottomCurve().polyline(),
                                                     m_depthPerception.value());
      dewarping::CylindricalSurfaceDewarper::State state;

      for (int j = 0; j < numVertGridLines; ++j) {
        const double x = j / (numVertGridLines - 1.0);
        const dewarping::CylindricalSurfaceDewarper::Generatrix gtx(dewarper.mapGeneratrix(x, state));
        const QPointF gtxP0(gtx.imgLine.pointAt(gtx.pln2img(0)));
        const QPointF gtxP1(gtx.imgLine.pointAt(gtx.pln2img(1)));
        painter.drawLine(gtxP0, gtxP1);
        for (int i = 0; i < numHorGridLines; ++i) {
          const double y = i / (numHorGridLines - 1.0);
          curves[i].push_back(gtx.imgLine.pointAt(gtx.pln2img(y)));
        }
      }

      for (const QVector<QPointF>& curve : curves) {
        painter.drawPolyline(curve);
      }
    } catch (const std::runtime_error&) {
      // Still probably a bad model, even though DistortionModel::isValid() was true.
      validModel = false;
    }
  }  // validModel
  if (!m_simpleManualEnabled && !validModel) {
    // Just draw the frame.
    const dewarping::Curve& topCurve = m_distortionModel.topCurve();
    const dewarping::Curve& bottomCurve = m_distortionModel.bottomCurve();
    painter.drawLine(topCurve.polyline().front(), bottomCurve.polyline().front());
    painter.drawLine(topCurve.polyline().back(), bottomCurve.polyline().back());
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
    painter.drawPolyline(QVector<QPointF>::fromStdVector(topCurve.polyline()));
    painter.drawPolyline(QVector<QPointF>::fromStdVector(bottomCurve.polyline()));
#else
    painter.drawPolyline(QVector<QPointF>(topCurve.polyline().begin(), topCurve.polyline().end()));
    painter.drawPolyline(QVector<QPointF>(bottomCurve.polyline().begin(), bottomCurve.polyline().end()));
#endif
  }

  if (m_simpleManualEnabled) {
    paintSimpleControls(painter, interaction);
  } else {
    paintXSpline(painter, interaction, m_topSpline);
    paintXSpline(painter, interaction, m_bottomSpline);
  }
}  // DewarpingView::onPaint

void DewarpingView::paintSimpleControls(QPainter& painter, const InteractionState& interaction) {
  const QColor curveColors[2] = {QColor(235, 120, 20), QColor(20, 155, 115)};
  const QColor borderColor(35, 115, 215);

  painter.save();
  painter.setBrush(Qt::NoBrush);
  for (int curveIdx = 0; curveIdx < 2; ++curveIdx) {
    const XSpline& spline = curveIdx == 0 ? m_topSpline.spline() : m_bottomSpline.spline();
    const std::vector<QPointF> polyline(spline.toPolyline());
    QPen curvePen(curveColors[curveIdx], 3.0);
    curvePen.setCosmetic(true);
    painter.setPen(curvePen);
    painter.drawPolyline(polyline.data(), static_cast<int>(polyline.size()));
  }

  QPen connectorPen(QColor(60, 115, 200, 150), 1.5, Qt::DashLine);
  connectorPen.setCosmetic(true);
  painter.setPen(connectorPen);
  for (int anchorIdx = 0; anchorIdx < 3; ++anchorIdx) {
    painter.drawLine(simpleAnchorPosition(0, anchorIdx), simpleAnchorPosition(1, anchorIdx));
  }
  QPen borderPen(borderColor, 3.0);
  borderPen.setCosmetic(true);
  painter.setPen(borderPen);
  for (int borderIdx = 0; borderIdx < 2; ++borderIdx) {
    painter.drawLine(simpleBorderPosition(borderIdx, 0), simpleBorderPosition(borderIdx, 1));
  }
  painter.restore();

  painter.save();
  painter.setWorldTransform(QTransform());
  for (int curveIdx = 0; curveIdx < 2; ++curveIdx) {
    for (int anchorIdx = 0; anchorIdx < 3; ++anchorIdx) {
      const QPointF center(simpleHandlePosition(curveIdx, anchorIdx));
      const bool highlighted = m_simpleHandles[curveIdx][anchorIdx].handler.proximityLeader(interaction);
      painter.setPen(QPen(Qt::white, highlighted ? 3.0 : 2.0));
      painter.setBrush(highlighted ? curveColors[curveIdx].lighter(125) : curveColors[curveIdx]);
      painter.drawEllipse(center, highlighted ? 11.0 : 9.0, highlighted ? 11.0 : 9.0);
      painter.setPen(Qt::white);
      painter.drawText(QRectF(center.x() - 9, center.y() - 9, 18, 18), Qt::AlignCenter,
                       QString::number(anchorIdx + 1));
    }
  }

  for (int borderIdx = 0; borderIdx < 2; ++borderIdx) {
    for (int curveIdx = 0; curveIdx < 2; ++curveIdx) {
      const QPointF center(simpleBorderHandlePosition(borderIdx, curveIdx));
      const bool highlighted = m_simpleBorderHandles[borderIdx][curveIdx].handler.proximityLeader(interaction);
      const double radius = highlighted ? 11.0 : 9.0;
      painter.setPen(QPen(Qt::white, highlighted ? 3.0 : 2.0));
      painter.setBrush(highlighted ? borderColor.lighter(125) : borderColor);
      painter.drawRoundedRect(QRectF(center.x() - radius, center.y() - radius, 2.0 * radius, 2.0 * radius),
                              3.0, 3.0);
      painter.setPen(Qt::white);
      painter.drawText(QRectF(center.x() - radius, center.y() - radius, 2.0 * radius, 2.0 * radius),
                       Qt::AlignCenter, borderIdx == 0 ? tr("L") : tr("R"));
    }
  }

  const QString instruction
      = tr("Simple manual: place orange 1-2-3 on an upper text line and green 1-2-3 on a lower line. "
           "Use the four blue L/R points for the left and right page borders.");
  const QRect availableRect(viewport()->rect().adjusted(12, 12, -12, -12));
  const QRect textRect(painter.fontMetrics().boundingRect(availableRect, Qt::TextWordWrap, instruction));
  const QRect panelRect(textRect.adjusted(-8, -5, 8, 5));
  painter.setPen(QPen(QColor(55, 75, 105), 1));
  painter.setBrush(QColor(255, 255, 255, 220));
  painter.drawRoundedRect(panelRect, 4, 4);
  painter.setPen(QColor(35, 50, 75));
  painter.drawText(textRect, Qt::TextWordWrap, instruction);
  painter.restore();
}

void DewarpingView::paintXSpline(QPainter& painter,
                                 const InteractionState& interaction,
                                 const InteractiveXSpline& ispline) {
  const XSpline& spline = ispline.spline();

  painter.save();
  painter.setBrush(Qt::NoBrush);

#if 0  // No point in drawing the curve itself - we already draw the grid.
        painter.setWorldTransform(imageToVirtual() * virtualToWidget());

        QPen curvePen(Qt::blue);
        curvePen.setWidthF(1.5);
        curvePen.setCosmetic(true);
        painter.setPen(curvePen);

        const std::vector<QPointF> polyline(spline.toPolyline());
        painter.drawPolyline(&polyline[0], polyline.size());
#endif
  // Drawing cosmetic points in transformed coordinates seems unreliable,
  // so let's draw them in widget coordinates.
  painter.setWorldMatrixEnabled(false);

  QPen existingPointPen(Qt::red);
  existingPointPen.setWidthF(4.0);
  existingPointPen.setCosmetic(true);
  painter.setPen(existingPointPen);

  const int numControlPoints = spline.numControlPoints();
  for (int i = 0; i < numControlPoints; ++i) {
    painter.drawPoint(sourceToWidget(spline.controlPointPosition(i)));
  }

  QPointF pt;
  if (ispline.curveIsProximityLeader(interaction, &pt)) {
    QPen newPointPen(existingPointPen);
    newPointPen.setColor(QColor(0x00ffff));
    painter.setPen(newPointPen);
    painter.drawPoint(pt);
  }

  painter.restore();
}  // DewarpingView::paintXSpline

QPointF DewarpingView::simpleAnchorPosition(const int curveIdx, const int anchorIdx) const {
  const XSpline& spline = curveIdx == 0 ? m_topSpline.spline() : m_bottomSpline.spline();
  constexpr double anchorParameters[3] = {0.18, 0.5, 0.82};
  return spline.pointAt(anchorParameters[qBound(0, anchorIdx, 2)]);
}

QPointF DewarpingView::simpleHandlePosition(const int curveIdx, const int anchorIdx) const {
  return sourceToWidget(simpleAnchorPosition(curveIdx, anchorIdx));
}

QPointF DewarpingView::simpleBorderPosition(const int borderIdx, const int curveIdx) const {
  const XSpline& spline = curveIdx == 0 ? m_topSpline.spline() : m_bottomSpline.spline();
  return spline.pointAt(borderIdx == 0 ? 0.0 : 1.0);
}

QPointF DewarpingView::simpleBorderHandlePosition(const int borderIdx, const int curveIdx) const {
  return sourceToWidget(simpleBorderPosition(borderIdx, curveIdx));
}

void DewarpingView::simpleHandleMoveRequest(const int curveIdx,
                                            const int anchorIdx,
                                            const QPointF& widgetPos,
                                            const Qt::KeyboardModifiers modifiers) {
  QPointF sourcePos(widgetToSource(widgetPos));
  const QPointF currentPos(simpleAnchorPosition(curveIdx, anchorIdx));
  if (modifiers.testFlag(Qt::ShiftModifier)) {
    sourcePos.setX(currentPos.x());
  }
  if (modifiers.testFlag(Qt::ControlModifier)) {
    sourcePos.setY(currentPos.y());
  }
  moveSimpleAnchor(curveIdx, anchorIdx, sourcePos);
}

void DewarpingView::simpleHandleDragFinished(const int curveIdx, const int anchorIdx) {
  const QPointF currentPos(simpleAnchorPosition(curveIdx, anchorIdx));
  const QPointF snappedPos(snapToNearbyTextLine(currentPos));
  if (QLineF(currentPos, snappedPos).length() > 0.5) {
    moveSimpleAnchor(curveIdx, anchorIdx, snappedPos);
  }
  dragFinished();
}

void DewarpingView::simpleBorderMoveRequest(const int borderIdx,
                                            const int curveIdx,
                                            const QPointF& widgetPos,
                                            const Qt::KeyboardModifiers modifiers) {
  QPointF sourcePos(widgetToSource(widgetPos));
  const QPointF currentPos(simpleBorderPosition(borderIdx, curveIdx));
  if (modifiers.testFlag(Qt::ShiftModifier)) {
    sourcePos.setX(currentPos.x());
  }
  if (modifiers.testFlag(Qt::ControlModifier)) {
    sourcePos.setY(currentPos.y());
  }
  moveSimpleBorderPoint(borderIdx, curveIdx, sourcePos);
}

void DewarpingView::simpleBorderDragFinished() {
  dragFinished();
}

void DewarpingView::moveSimpleAnchor(const int curveIdx, const int anchorIdx, const QPointF& sourcePos) {
  InteractiveXSpline& interactiveSpline = curveIdx == 0 ? m_topSpline : m_bottomSpline;
  XSpline spline(interactiveSpline.spline());
  if (spline.numControlPoints() < 2) {
    return;
  }

  constexpr double anchorParameters[3] = {0.18, 0.5, 0.82};
  const double anchorT = anchorParameters[qBound(0, anchorIdx, 2)];
  // Apply a smooth displacement field instead of moving a single technical
  // spline point. This makes the three visible handles behave like the left,
  // middle and right portions of the printed line while preserving details
  // from a good automatic model.
  for (int correctionPass = 0; correctionPass < 3; ++correctionPass) {
    const QPointF correction(sourcePos - spline.pointAt(anchorT));
    if (QLineF(QPointF(), correction).length() < 0.05) {
      break;
    }
    const int numControlPoints = spline.numControlPoints();
    for (int pointIdx = 0; pointIdx < numControlPoints; ++pointIdx) {
      const double t = spline.controlPointIndexToT(pointIdx);
      // Keep the four page-border corners stationary while the numbered
      // handles shape the text lines inside them.
      constexpr double sigma = 0.18;
      const double distance = (t - anchorT) / sigma;
      const double edgeWindow = 4.0 * t * (1.0 - t);
      const double anchorWindow = 4.0 * anchorT * (1.0 - anchorT);
      const double weight = std::exp(-0.5 * distance * distance) * edgeWindow / anchorWindow;
      spline.moveControlPoint(pointIdx, spline.controlPointPosition(pointIdx) + weight * correction);
    }
  }
  interactiveSpline.setSpline(spline);
}

void DewarpingView::moveSimpleBorderPoint(const int borderIdx,
                                          const int curveIdx,
                                          const QPointF& sourcePos) {
  InteractiveXSpline& interactiveSpline = curveIdx == 0 ? m_topSpline : m_bottomSpline;
  XSpline spline(interactiveSpline.spline());
  if (spline.numControlPoints() < 2) {
    return;
  }
  const int pointIdx = borderIdx == 0 ? 0 : spline.numControlPoints() - 1;
  spline.moveControlPoint(pointIdx, sourcePos);
  interactiveSpline.setSpline(spline);
}

QPointF DewarpingView::snapToNearbyTextLine(const QPointF& sourcePos) const {
  if (m_previewSourceImage.isNull()) {
    return sourcePos;
  }

  const QPointF previewPos(m_sourceToPreview.map(sourcePos));
  const int centerX = qRound(previewPos.x());
  const int centerY = qRound(previewPos.y());
  const int searchRadius = qBound(6, m_previewSourceImage.height() / 60, 22);
  const int halfWidth = qBound(18, m_previewSourceImage.width() / 24, 60);
  const int left = qBound(0, centerX - halfWidth, m_previewSourceImage.width() - 1);
  const int right = qBound(0, centerX + halfWidth, m_previewSourceImage.width() - 1);
  const int top = qBound(0, centerY - searchRadius, m_previewSourceImage.height() - 1);
  const int bottom = qBound(0, centerY + searchRadius, m_previewSourceImage.height() - 1);

  double bestScore = 0.0;
  int bestY = centerY;
  int samplesPerRow = 0;
  for (int y = top; y <= bottom; ++y) {
    double score = 0.0;
    int samples = 0;
    for (int x = left; x <= right; x += 2) {
      const int gray = qGray(m_previewSourceImage.pixel(x, y));
      score += std::max(0, 210 - gray);
      ++samples;
    }
    samplesPerRow = samples;
    if (score > bestScore) {
      bestScore = score;
      bestY = y;
    }
  }

  // Avoid snapping to random noise or an empty paper background.
  if ((samplesPerRow == 0) || (bestScore < samplesPerRow * 12.0)) {
    return sourcePos;
  }

  const QTransform previewToSource(m_sourceToPreview.inverted());
  const QPointF snappedSource(previewToSource.map(QPointF(previewPos.x(), bestY)));
  return QPointF(sourcePos.x(), snappedSource.y());
}

void DewarpingView::setSimpleHandlersLinked(const bool linked) {
  if (linked) {
    m_topSpline.unlink();
    m_bottomSpline.unlink();
    for (auto& curveHandles : m_simpleHandles) {
      for (SimpleHandle& handle : curveHandles) {
        makeLastFollower(handle.handler);
      }
    }
    for (auto& borderHandles : m_simpleBorderHandles) {
      for (SimpleHandle& handle : borderHandles) {
        makeLastFollower(handle.handler);
      }
    }
  } else {
    for (auto& curveHandles : m_simpleHandles) {
      for (SimpleHandle& handle : curveHandles) {
        handle.handler.unlink();
      }
    }
    for (auto& borderHandles : m_simpleBorderHandles) {
      for (SimpleHandle& handle : borderHandles) {
        handle.handler.unlink();
      }
    }
    makeLastFollower(m_topSpline);
    makeLastFollower(m_bottomSpline);
  }
}

void DewarpingView::curveModified(int curveIdx) {
  if (curveIdx == 0) {
    m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
  } else {
    m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));
  }
  scheduleLivePreview();
  update();
}

void DewarpingView::dragFinished() {
  if ((m_dewarpingOptions.dewarpingMode() == AUTO) || (m_dewarpingOptions.dewarpingMode() == AUTO_BOOK)
      || (m_dewarpingOptions.dewarpingMode() == MARGINAL)
      || (m_dewarpingOptions.dewarpingMode() == AUTO_TABLE)) {
    m_dewarpingOptions.setDewarpingMode(MANUAL);
  }
  emit distortionModelChanged(m_distortionModel);
}

/** Source image coordinates to widget coordinates. */
QPointF DewarpingView::sourceToWidget(const QPointF& pt) const {
  return virtualToWidget().map(imageToVirtual().map(pt));
}

/** Widget coordinates to source image coordinates. */
QPointF DewarpingView::widgetToSource(const QPointF& pt) const {
  return virtualToImage().map(widgetToVirtual().map(pt));
}

QPolygonF DewarpingView::virtMarginArea(int marginIdx) const {
  const dewarping::Curve& topCurve = m_distortionModel.topCurve();
  const dewarping::Curve& bottomCurve = m_distortionModel.bottomCurve();

  QLineF vertBoundary;   // From top to bottom, that's important!
  if (marginIdx == 0) {  // Left margin.
    vertBoundary.setP1(topCurve.polyline().front());
    vertBoundary.setP2(bottomCurve.polyline().front());
  } else {  // Right margin.
    vertBoundary.setP1(topCurve.polyline().back());
    vertBoundary.setP2(bottomCurve.polyline().back());
  }

  vertBoundary = imageToVirtual().map(vertBoundary);

  QLineF normal;
  if (marginIdx == 0) {  // Left margin.
    normal = QLineF(vertBoundary.p2(), vertBoundary.p1()).normalVector();
  } else {  // Right margin.
    normal = vertBoundary.normalVector();
  }

  // Project every vertex in the m_virtDisplayArea polygon
  // to vert_line and to its normal, keeping track min and max values.
  double min = NumericTraits<double>::max();
  double max = NumericTraits<double>::min();
  double normalMax = max;
  const ToLineProjector vertLineProjector(vertBoundary);
  const ToLineProjector normalProjector(normal);
  for (const QPointF& pt : m_virtDisplayArea) {
    const double p1 = vertLineProjector.projectionScalar(pt);
    if (p1 < min) {
      min = p1;
    }
    if (p1 > max) {
      max = p1;
    }

    const double p2 = normalProjector.projectionScalar(pt);
    if (p2 > normalMax) {
      normalMax = p2;
    }
  }

  // Workaround clipping bugs in QPolygon::intersected().
  min -= 1.0;
  max += 1.0;
  normalMax += 1.0;

  QPolygonF poly;
  poly << vertBoundary.pointAt(min);
  poly << vertBoundary.pointAt(max);
  poly << vertBoundary.pointAt(max) + normal.pointAt(normalMax) - normal.p1();
  poly << vertBoundary.pointAt(min) + normal.pointAt(normalMax) - normal.p1();
  return m_virtDisplayArea.intersected(poly);
}  // DewarpingView::virtMarginArea
}  // namespace output
