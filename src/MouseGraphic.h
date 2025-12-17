#pragma once

#include <QVector>
#include <QImage>

#include "Mouse.h"
#include "TriangleGraphic.h"
#include "Polygon.h"
#include "units/Coordinate.h"
#include "Direction.h"
#include "RGB.h"
#include "units/Distance.h"
#include "VertexGraphic.h"

namespace mms {

class MouseGraphic {
 public:
  MouseGraphic(const Mouse *mouse);
  QVector<TriangleGraphic> draw() const;

 private:
  void drawAllPixels(QVector<TriangleGraphic> &buffer,
                     qreal centerX, qreal centerY,
                     qreal pixelWidth, qreal pixelHeight,
                     qreal rotationRad) const;
  void drawNonTransparentPixels(QVector<TriangleGraphic> &buffer,
                                qreal centerX, qreal centerY,
                                qreal pixelWidth, qreal pixelHeight,
                                qreal rotationRad) const;
  void drawOptimized(QVector<TriangleGraphic> &buffer,
                     qreal centerX, qreal centerY,
                     qreal pixelWidth, qreal pixelHeight,
                     qreal rotationRad) const;
  TriangleGraphic createTriangle(const Coordinate &v1,
                                 const Coordinate &v2,
                                 const Coordinate &v3,
                                 const RGB &color,
                                 int alpha) const;

  const Mouse *m_mouse;
  bool m_imageLoaded;
  QImage m_image;
  int m_imageWidth;
  int m_imageHeight;
};

}  // namespace mms
