#pragma once

#include <QVector>
#include <QImage>

#include "TriangleGraphic.h"
#include "units/Coordinate.h"
#include "RGB.h"
#include "units/Distance.h"
#include "VertexGraphic.h"

namespace mms {

class CheeseGraphic {
 public:
  CheeseGraphic(const Coordinate& position, qreal scale = 1.0);
  QVector<TriangleGraphic> draw() const;

 private:
  void drawAllPixels(QVector<TriangleGraphic> &buffer,
                     qreal centerX, qreal centerY,
                     qreal pixelWidth, qreal pixelHeight) const;
  TriangleGraphic createTriangle(const Coordinate &v1,
                                 const Coordinate &v2,
                                 const Coordinate &v3,
                                 const RGB &color,
                                 int alpha) const;

  Coordinate m_position;
  qreal m_scale;
  bool m_imageLoaded;
  QImage m_image;
  int m_imageWidth;
  int m_imageHeight;
};

}  // namespace mms