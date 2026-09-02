// Copyright (C) 2026 ScanTailor FanMode contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TableLineDistortionModelBuilder.h"

#include <GrayImage.h>
#include <QPointF>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "Curve.h"

using imageproc::GrayImage;

namespace dewarping {
namespace {
struct RuleCandidate {
  double centerY = 0.0;
  double slope = 0.0;
  int left = 0;
  int right = 0;
  double density = 0.0;
  double score = 0.0;
};

struct Run {
  int start = -1;
  int end = -1;
  int support = 0;

  int sampleLength() const { return (start < 0) ? 0 : end - start + 1; }
};

uint8_t pixel(const GrayImage& image, const int x, const int y) {
  return image.data()[y * image.stride() + x];
}

bool darkNear(const GrayImage& image, const int x, const int y, const int tolerance, const int threshold) {
  const int top = std::max(0, y - tolerance);
  const int bottom = std::min(image.height() - 1, y + tolerance);
  for (int yy = top; yy <= bottom; ++yy) {
    if (pixel(image, x, yy) <= threshold) {
      return true;
    }
  }
  return false;
}

Run longestSupportedRun(const std::vector<bool>& supported, const int maxGap) {
  Run best;
  Run current;
  int lastSupport = -1;

  const auto finishCurrent = [&]() {
    if ((current.sampleLength() > best.sampleLength())
        || ((current.sampleLength() == best.sampleLength()) && (current.support > best.support))) {
      best = current;
    }
  };

  for (int i = 0; i < static_cast<int>(supported.size()); ++i) {
    if (!supported[i]) {
      continue;
    }
    if ((lastSupport >= 0) && (i - lastSupport - 1 > maxGap)) {
      finishCurrent();
      current = Run();
    }
    if (current.start < 0) {
      current.start = i;
    }
    current.end = i;
    ++current.support;
    lastSupport = i;
  }
  finishCurrent();
  return best;
}

double yAt(const RuleCandidate& rule, const double x, const double centerX) {
  return rule.centerY + rule.slope * (x - centerX);
}

double overlap(const RuleCandidate& first, const RuleCandidate& second) {
  return std::max(0, std::min(first.right, second.right) - std::max(first.left, second.left));
}

std::vector<QPointF> traceRule(const GrayImage& image,
                               const QRect& rect,
                               const RuleCandidate& rule,
                               const int verticalSearch) {
  constexpr int pointCount = 25;
  std::vector<double> rawY(pointCount);
  std::vector<double> medianY(pointCount);
  std::vector<QPointF> result;
  result.reserve(pointCount);

  const double centerX = rect.center().x();
  const int horizontalRadius = std::max(4, rect.width() / 240);
  for (int i = 0; i < pointCount; ++i) {
    const double fraction = static_cast<double>(i) / (pointCount - 1);
    const int x = qRound(rule.left + fraction * (rule.right - rule.left));
    const int predictedY = qRound(yAt(rule, x, centerX));
    int bestY = predictedY;
    int bestScore = std::numeric_limits<int>::min();

    for (int y = std::max(rect.top(), predictedY - verticalSearch);
         y <= std::min(rect.bottom(), predictedY + verticalSearch); ++y) {
      int score = 0;
      for (int xx = std::max(rect.left(), x - horizontalRadius);
           xx <= std::min(rect.right(), x + horizontalRadius); ++xx) {
        uint8_t darkest = 255;
        for (int yy = std::max(rect.top(), y - 1); yy <= std::min(rect.bottom(), y + 1); ++yy) {
          darkest = std::min(darkest, pixel(image, xx, yy));
        }
        score += 255 - darkest;
      }
      // Resolve ambiguous intersections in favour of the predicted rule.
      score -= 12 * std::abs(y - predictedY);
      if (score > bestScore) {
        bestScore = score;
        bestY = y;
      }
    }
    rawY[i] = bestY;
  }

  // A short median filter removes the spikes caused by vertical cell borders,
  // while retaining the low-frequency bow of the scanned sheet.
  for (int i = 0; i < pointCount; ++i) {
    std::vector<double> neighbourhood;
    for (int j = std::max(0, i - 2); j <= std::min(pointCount - 1, i + 2); ++j) {
      neighbourhood.push_back(rawY[j]);
    }
    std::sort(neighbourhood.begin(), neighbourhood.end());
    medianY[i] = neighbourhood[neighbourhood.size() / 2];
  }

  for (int i = 0; i < pointCount; ++i) {
    double weightedY = 0.0;
    double totalWeight = 0.0;
    for (int j = std::max(0, i - 2); j <= std::min(pointCount - 1, i + 2); ++j) {
      const double weight = 3.0 - std::abs(i - j);
      weightedY += weight * medianY[j];
      totalWeight += weight;
    }
    const double fraction = static_cast<double>(i) / (pointCount - 1);
    result.emplace_back(rule.left + fraction * (rule.right - rule.left), weightedY / totalWeight);
  }
  return result;
}
}  // namespace

TableLineDistortionModelBuilder::Result TableLineDistortionModelBuilder::build(const GrayImage& image,
                                                                                const QRect& contentRect) {
  Result result;
  const QRect rect = contentRect.intersected(image.rect());
  if (image.isNull() || (rect.width() < 160) || (rect.height() < 160)) {
    return result;
  }

  const int xStep = std::max(3, rect.width() / 340);
  const int verticalTolerance = std::max(3, std::min(7, rect.height() / 420));
  const int threshold = 180;
  const int sampleCount = 1 + rect.width() / xStep;
  const double centerX = rect.center().x();
  std::vector<RuleCandidate> candidates;

  // A line is allowed a small slope here. Curvature is recovered later by
  // tracing the actual ink around the winning line hypotheses.
  constexpr double minSlope = -0.035;
  constexpr double maxSlope = 0.035;
  constexpr double slopeStep = 0.0025;
  for (int centerY = rect.top(); centerY <= rect.bottom(); centerY += 2) {
    RuleCandidate bestForRow;
    for (double slope = minSlope; slope <= maxSlope + 0.0001; slope += slopeStep) {
      std::vector<bool> supported;
      supported.reserve(sampleCount);
      for (int x = rect.left(); x <= rect.right(); x += xStep) {
        const int y = qRound(centerY + slope * (x - centerX));
        supported.push_back((y >= rect.top()) && (y <= rect.bottom())
                            && darkNear(image, x, y, verticalTolerance, threshold));
      }

      const Run run = longestSupportedRun(supported, 2);
      if (run.sampleLength() == 0) {
        continue;
      }
      const int left = rect.left() + run.start * xStep;
      const int right = std::min(rect.right(), rect.left() + run.end * xStep);
      const double spanRatio = static_cast<double>(right - left) / rect.width();
      const double density = static_cast<double>(run.support) / run.sampleLength();
      const double score = spanRatio * density;
      if ((spanRatio >= 0.42) && (density >= 0.68) && (score > bestForRow.score)) {
        bestForRow = RuleCandidate{static_cast<double>(centerY), slope, left, right, density, score};
      }
    }
    if (bestForRow.score > 0.0) {
      candidates.push_back(bestForRow);
    }
  }
  result.candidateLineCount = static_cast<int>(candidates.size());

  // Suppress duplicate hypotheses produced by a thick or antialiased rule.
  std::sort(candidates.begin(), candidates.end(), [](const RuleCandidate& lhs, const RuleCandidate& rhs) {
    return lhs.score > rhs.score;
  });
  std::vector<RuleCandidate> unique;
  for (const RuleCandidate& candidate : candidates) {
    bool duplicate = false;
    for (const RuleCandidate& accepted : unique) {
      const double sharedCenter = 0.5 * (std::max(candidate.left, accepted.left)
                                         + std::min(candidate.right, accepted.right));
      if ((overlap(candidate, accepted) > 0.35 * rect.width())
          && (std::abs(yAt(candidate, sharedCenter, centerX) - yAt(accepted, sharedCenter, centerX)) < 8.0)) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      unique.push_back(candidate);
    }
    if (unique.size() >= 80) {
      break;
    }
  }

  // Find the largest coherent rule family. Similar endpoints are important:
  // this excludes ordinary underlined headings and Arabic text baselines.
  std::vector<RuleCandidate> bestFamily;
  double bestFamilyScore = 0.0;
  for (const RuleCandidate& anchor : unique) {
    std::vector<RuleCandidate> family;
    for (const RuleCandidate& candidate : unique) {
      const int anchorWidth = anchor.right - anchor.left;
      const int candidateWidth = candidate.right - candidate.left;
      const double widthRatio = static_cast<double>(std::min(anchorWidth, candidateWidth))
                                / std::max(anchorWidth, candidateWidth);
      const double overlapRatio = overlap(anchor, candidate) / std::min(anchorWidth, candidateWidth);
      if ((widthRatio >= 0.70) && (overlapRatio >= 0.82)
          && (std::abs(anchor.left - candidate.left) <= 0.13 * rect.width())
          && (std::abs(anchor.right - candidate.right) <= 0.13 * rect.width())
          && (std::abs(anchor.slope - candidate.slope) <= 0.025)) {
        family.push_back(candidate);
      }
    }
    if (family.empty()) {
      continue;
    }
    const auto vertical = std::minmax_element(family.begin(), family.end(), [](const RuleCandidate& lhs,
                                                                               const RuleCandidate& rhs) {
      return lhs.centerY < rhs.centerY;
    });
    const double spread = vertical.second->centerY - vertical.first->centerY;
    const double familyScore = family.size() + spread / rect.height();
    if (familyScore > bestFamilyScore) {
      bestFamilyScore = familyScore;
      bestFamily = std::move(family);
    }
  }

  if (bestFamily.size() < 3) {
    return result;
  }
  std::sort(bestFamily.begin(), bestFamily.end(), [](const RuleCandidate& lhs, const RuleCandidate& rhs) {
    return lhs.centerY < rhs.centerY;
  });
  const RuleCandidate& top = bestFamily.front();
  const RuleCandidate& bottom = bestFamily.back();
  if (bottom.centerY - top.centerY < std::max(30.0, 0.07 * rect.height())) {
    return result;
  }

  const int traceSearch = verticalTolerance + 7;
  const std::vector<QPointF> topCurve = traceRule(image, rect, top, traceSearch);
  const std::vector<QPointF> bottomCurve = traceRule(image, rect, bottom, traceSearch);
  result.model.setTopCurve(Curve(topCurve));
  result.model.setBottomCurve(Curve(bottomCurve));
  result.acceptedLineCount = static_cast<int>(bestFamily.size());
  result.reliable = result.model.isValid();
  if (!result.reliable) {
    result.model = DistortionModel();
  }
  return result;
}
}  // namespace dewarping
