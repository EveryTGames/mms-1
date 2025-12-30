#include "CheeseGraphic.h"

#include <QImage>
#include <QtMath>
#include <QDebug>
#include <limits>
#include <cmath>
#include <QPointF>
#include <algorithm>

#include "SimUtilities.h"
#include "Color.h"

namespace mms
{

CheeseGraphic::CheeseGraphic(const Coordinate& position, qreal scale)
    : m_position(position), m_scale(scale)
{
    // Load the cheese image file
    QImage img("://resources/icons/cheese.png"); // Assuming cheese.png exists in resources

    if (img.isNull())
    {
        qWarning("Cheese image failed to load, using default polygon");
        m_imageLoaded = false;
        return;
    }

    m_imageLoaded = true;

    // Convert to RGBA format for easier pixel access
    m_image = img.convertToFormat(QImage::Format_RGBA8888);

    // Store dimensions
    m_imageWidth = m_image.width();
    m_imageHeight = m_image.height();
}

QVector<TriangleGraphic> CheeseGraphic::draw() const
{
    QVector<TriangleGraphic> buffer;

    if (!m_imageLoaded)
    {
        // Fallback to simple yellow square for cheese
        qreal size = 0.05 * m_scale; // 5cm square
        Coordinate topLeft = Coordinate::Cartesian(
            Distance::Meters(m_position.getX().getMeters() - size/2),
            Distance::Meters(m_position.getY().getMeters() - size/2));
        Coordinate topRight = Coordinate::Cartesian(
            Distance::Meters(m_position.getX().getMeters() + size/2),
            Distance::Meters(m_position.getY().getMeters() - size/2));
        Coordinate bottomRight = Coordinate::Cartesian(
            Distance::Meters(m_position.getX().getMeters() + size/2),
            Distance::Meters(m_position.getY().getMeters() + size/2));
        Coordinate bottomLeft = Coordinate::Cartesian(
            Distance::Meters(m_position.getX().getMeters() - size/2),
            Distance::Meters(m_position.getY().getMeters() + size/2));

        RGB yellow = RGB{255, 255, 0}; // Yellow color for cheese
        buffer.append(createTriangle(topLeft, topRight, bottomRight, yellow, 255));
        buffer.append(createTriangle(topLeft, bottomRight, bottomLeft, yellow, 255));
        return buffer;
    }

    // Get center position
    qreal centerX = m_position.getX().getMeters();
    qreal centerY = m_position.getY().getMeters();

    // Set pixel size based on scale
    const qreal METERS_PER_PIXEL = 0.005 * m_scale; // Base 5mm per pixel, scaled

    qreal pixelWidth = METERS_PER_PIXEL;
    qreal pixelHeight = METERS_PER_PIXEL;

    // Draw the image
    drawAllPixels(buffer, centerX, centerY, pixelWidth, pixelHeight);

    return buffer;
}

void CheeseGraphic::drawAllPixels(QVector<TriangleGraphic> &buffer,
                                 qreal centerX, qreal centerY,
                                 qreal pixelWidth, qreal pixelHeight) const
{
    // Calculate total image dimensions
    qreal totalWidth = m_imageWidth * pixelWidth;
    qreal totalHeight = m_imageHeight * pixelHeight;

    // Debug output to see image size
    qDebug() << "Drawing cheese at center:" << centerX << "," << centerY
             << "with size:" << totalWidth << "x" << totalHeight
             << "meters, pixel size:" << pixelWidth << "x" << pixelHeight
             << "image dimensions:" << m_imageWidth << "x" << m_imageHeight;

    // Iterate through every pixel in the image
    for (int y = 0; y < m_imageHeight; y++)
    {
        for (int x = 0; x < m_imageWidth; x++)
        {
            // Get pixel color
            QRgb pixel = m_image.pixel(x, y);
            int alpha = qAlpha(pixel);

            // Skip fully transparent pixels
            if (alpha == 0)
                continue;

            // Convert from image coordinates to local coordinates
            // Center the image at (0,0) in local space
            qreal localX = (x - m_imageWidth / 2.0) * pixelWidth;
            qreal localY = (y - m_imageHeight / 2.0) * pixelHeight;

            // Create a pixel quad (2 triangles)
            QVector<QPointF> pixelCorners = {
                QPointF(localX, localY),                     // top-left
                QPointF(localX + pixelWidth, localY),        // top-right
                QPointF(localX + pixelWidth, localY + pixelHeight), // bottom-right
                QPointF(localX, localY + pixelHeight)        // bottom-left
            };

            // Translate each corner to world position (no rotation for static cheese)
            QVector<Coordinate> worldCorners;
            for (const auto &corner : pixelCorners)
            {
                worldCorners.append(Coordinate::Cartesian(
                    Distance::Meters(corner.x() + centerX),
                    Distance::Meters(corner.y() + centerY)));
            }

            // Create two triangles for this pixel
            RGB color = RGB{qRed(pixel), qGreen(pixel), qBlue(pixel)};

            // First triangle
            buffer.append(createTriangle(worldCorners[0], worldCorners[1], worldCorners[2], color, alpha));

            // Second triangle
            buffer.append(createTriangle(worldCorners[0], worldCorners[2], worldCorners[3], color, alpha));
        }
    }
}

TriangleGraphic CheeseGraphic::createTriangle(const Coordinate &v1,
                                             const Coordinate &v2,
                                             const Coordinate &v3,
                                             const RGB &color,
                                             int alpha) const
{
    auto makeVertex = [&](const Coordinate &c) -> VertexGraphic
    {
        VertexGraphic v;
        v.x = static_cast<float>(c.getX().getMeters());
        v.y = static_cast<float>(c.getY().getMeters());
        v.rgb = color;
        v.a = alpha;
        return v;
    };

    return TriangleGraphic{makeVertex(v1), makeVertex(v2), makeVertex(v3)};
}

}  // namespace mms