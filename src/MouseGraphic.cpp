#include "MouseGraphic.h"

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

MouseGraphic::MouseGraphic(const Mouse *mouse) : m_mouse(mouse) 
{
    // Load the image file
    QImage img("://resources/icons/theMouse.png"); // or any other image path
    
    if (img.isNull())
    {
        qWarning("Mouse image failed to load, using default polygon");
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

QVector<TriangleGraphic> MouseGraphic::draw() const 
{
    QVector<TriangleGraphic> buffer;
    
    if (!m_imageLoaded)
    {
        // Fallback to simple polygon
        buffer.append(SimUtilities::polygonToTriangleGraphics(
            m_mouse->getCurrentBodyPolygon(),
            Color::GRAY, 255));
        return buffer;
    }
    
    // Get mouse body polygon for positioning
    Polygon bodyPoly = m_mouse->getCurrentBodyPolygon();
    QVector<Coordinate> bodyVerts = bodyPoly.getVertices();
    if (bodyVerts.isEmpty())
        return buffer;
    
    // Calculate body center for positioning
    qreal centerX = 0.0, centerY = 0.0;
    for (const auto &v : bodyVerts)
    {
        centerX += v.getX().getMeters();
        centerY += v.getY().getMeters();
    }
    centerX /= bodyVerts.size();
    centerY /= bodyVerts.size();
    
    // Get mouse rotation
    SemiDirection dir = m_mouse->getCurrentDiscretizedRotation();
    qreal rotationDeg = 0.0;
    switch (dir)
    {
    case SemiDirection::EAST: rotationDeg = 0; break;
    case SemiDirection::NORTHEAST: rotationDeg = 45; break;
    case SemiDirection::NORTH: rotationDeg = 90; break;
    case SemiDirection::NORTHWEST: rotationDeg = 135; break;
    case SemiDirection::WEST: rotationDeg = 180; break;
    case SemiDirection::SOUTHWEST: rotationDeg = 225; break;
    case SemiDirection::SOUTH: rotationDeg = 270; break;
    case SemiDirection::SOUTHEAST: rotationDeg = 315; break;
    }
    
    qreal rad = qDegreesToRadians(rotationDeg);
    
    // **MAKE IMAGE LARGER**: Use a fixed scale instead of fitting to body polygon
    // For example, make each pixel 0.005 meters (5mm) which is 50x larger than before
    const qreal METERS_PER_PIXEL = 0.008; // Adjust this value to make image bigger/smaller
    
    qreal pixelWidth = METERS_PER_PIXEL;
    qreal pixelHeight = METERS_PER_PIXEL;
    
    // **OR**: Scale the image relative to mouse body size
    // qreal scaleFactor = 2.0; // Make image 2x larger than mouse body
    // qreal imageWidth = (bodyWidth * scaleFactor);
    // qreal imageHeight = (bodyHeight * scaleFactor);
    // qreal pixelWidth = imageWidth / m_imageWidth;
    // qreal pixelHeight = imageHeight / m_imageHeight;
    
    // Draw the image
    drawAllPixels(buffer, centerX, centerY, pixelWidth, pixelHeight, rad);
    
    return buffer;
}

void MouseGraphic::drawAllPixels(QVector<TriangleGraphic> &buffer,
                                 qreal centerX, qreal centerY,
                                 qreal pixelWidth, qreal pixelHeight,
                                 qreal rotationRad) const
{
    // Calculate total image dimensions
    qreal totalWidth = m_imageWidth * pixelWidth;
    qreal totalHeight = m_imageHeight * pixelHeight;
    
    // Debug output to see image size
    qDebug() << "Drawing image at center:" << centerX << "," << centerY
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
            
            // Rotate and translate each corner
            QVector<Coordinate> worldCorners;
            for (const auto &corner : pixelCorners)
            {
                qreal x_rot = corner.x() * std::cos(rotationRad) - corner.y() * std::sin(rotationRad);
                qreal y_rot = corner.x() * std::sin(rotationRad) + corner.y() * std::cos(rotationRad);
                
                worldCorners.append(Coordinate::Cartesian(
                    Distance::Meters(x_rot + centerX),
                    Distance::Meters(y_rot + centerY)));
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

// Optional: More efficient version that combines adjacent same-colored pixels
void MouseGraphic::drawOptimized(QVector<TriangleGraphic> &buffer,
                                 qreal centerX, qreal centerY,
                                 qreal pixelWidth, qreal pixelHeight,
                                 qreal rotationRad) const
{
    // This combines adjacent pixels with similar colors into larger rectangles
    // Much more efficient for large images
    
    // We'll process the image in 4x4 blocks
    const int BLOCK_SIZE = 4;
    
    for (int blockY = 0; blockY < m_imageHeight; blockY += BLOCK_SIZE)
    {
        for (int blockX = 0; blockX < m_imageWidth; blockX += BLOCK_SIZE)
        {
            // Check if this block has any non-transparent pixels
            bool hasVisiblePixels = false;
            int avgR = 0, avgG = 0, avgB = 0, avgA = 0;
            int pixelCount = 0;
            
            for (int y = blockY; y < std::min(blockY + BLOCK_SIZE, m_imageHeight); y++)
            {
                for (int x = blockX; x < std::min(blockX + BLOCK_SIZE, m_imageWidth); x++)
                {
                    QRgb pixel = m_image.pixel(x, y);
                    int alpha = qAlpha(pixel);
                    
                    if (alpha > 0)
                    {
                        hasVisiblePixels = true;
                        avgR += qRed(pixel);
                        avgG += qGreen(pixel);
                        avgB += qBlue(pixel);
                        avgA += alpha;
                        pixelCount++;
                    }
                }
            }
            
            if (!hasVisiblePixels || pixelCount == 0)
                continue;
            
            // Calculate average color for the block
            avgR /= pixelCount;
            avgG /= pixelCount;
            avgB /= pixelCount;
            avgA /= pixelCount;
            
            // Calculate block position and size
            qreal localX = (blockX - m_imageWidth / 2.0) * pixelWidth;
            qreal localY = (blockY - m_imageHeight / 2.0) * pixelHeight;
            qreal blockWidth = std::min(BLOCK_SIZE, m_imageWidth - blockX) * pixelWidth;
            qreal blockHeight = std::min(BLOCK_SIZE, m_imageHeight - blockY) * pixelHeight;
            
            QVector<QPointF> blockCorners = {
                QPointF(localX, localY),
                QPointF(localX + blockWidth, localY),
                QPointF(localX + blockWidth, localY + blockHeight),
                QPointF(localX, localY + blockHeight)
            };
            
            // Rotate and translate
            QVector<Coordinate> worldCorners;
            for (const auto &corner : blockCorners)
            {
                qreal x_rot = corner.x() * std::cos(rotationRad) - corner.y() * std::sin(rotationRad);
                qreal y_rot = corner.x() * std::sin(rotationRad) + corner.y() * std::cos(rotationRad);
                
                worldCorners.append(Coordinate::Cartesian(
                    Distance::Meters(x_rot + centerX),
                    Distance::Meters(y_rot + centerY)));
            }
            
            RGB color = RGB{avgR, avgG, avgB};
            buffer.append(createTriangle(worldCorners[0], worldCorners[1], worldCorners[2], color, avgA));
            buffer.append(createTriangle(worldCorners[0], worldCorners[2], worldCorners[3], color, avgA));
        }
    }
}

TriangleGraphic MouseGraphic::createTriangle(const Coordinate &v1, 
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