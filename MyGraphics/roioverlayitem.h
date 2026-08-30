#ifndef ROIOVERLAYITEM_H
#define ROIOVERLAYITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

class RoiOverlayItem : public QGraphicsItem
{
public:
    explicit RoiOverlayItem(QGraphicsItem *parent = nullptr)
        : QGraphicsItem(parent) {}

    void setImageRect(const QRectF &rect)
    {
        prepareGeometryChange();
        m_imageRect = rect;
    }

    void setRoiRect(const QRectF &rect)
    {
        prepareGeometryChange();
        m_roiRect = rect;
    }

    QRectF boundingRect() const override
    {
        return m_imageRect;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        if (m_imageRect.isNull())
            return;

        painter->setRenderHint(QPainter::Antialiasing);

        // 框外虚化：用半透明黑色覆盖ROI外部区域
        QPainterPath overlayPath;
        overlayPath.addRect(m_imageRect);
        if (!m_roiRect.isNull()) {
            QPainterPath roiPath;
            roiPath.addRect(m_roiRect);
            overlayPath = overlayPath.subtracted(roiPath);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 120));
        painter->drawPath(overlayPath);

        // ROI黑色虚线边框
        if (!m_roiRect.isNull()) {
            QPen borderPen(Qt::black, 2, Qt::DashLine);
            painter->setPen(borderPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(m_roiRect);
        }
    }

private:
    QRectF m_imageRect;
    QRectF m_roiRect;
};

#endif // ROIOVERLAYITEM_H
