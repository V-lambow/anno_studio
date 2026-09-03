#ifndef RECTITEM_H
#define RECTITEM_H

#include <QGraphicsRectItem>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QCursor>

class RectItem : public QGraphicsRectItem
{
public:
    enum { Type = UserType + 2 };

    enum Handle { None, TopLeft, TopRight, BottomLeft, BottomRight, Top, Bottom, Left, Right, Interior };

    RectItem(QColor color, int penSize, int classId, QGraphicsItem *parent = nullptr);

    int type() const override { return Type; }
    int classId() const { return m_classId; }
    QColor itemColor() const { return m_color; }
    int penWidth() const { return m_penWidth; }

    void setRectFromPoints(const QPointF &start, const QPointF &end);
    void setBoundary(const QRectF &boundary) { m_boundary = boundary; }
    QVector<QPointF> getCornerPoints() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    Handle handleAt(const QPointF &pos) const;
    void updateCursor(Handle h);
    QRectF clampRectToBoundary(const QRectF &r) const;

    int m_classId;
    QColor m_color;
    int m_penWidth;
    Handle m_activeHandle = None;
    QPointF m_pressPos;
    QRectF m_pressRect;
    QRectF m_boundary;
    qreal m_handleSize = 8.0;
};

#endif // RECTITEM_H
