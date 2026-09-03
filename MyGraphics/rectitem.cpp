#include "rectitem.h"
#include <QGraphicsScene>

RectItem::RectItem(QColor color, int penSize, int classId, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_classId(classId), m_color(color), m_penWidth(penSize)
{
    setPen(QPen(color, penSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setBrush(Qt::NoBrush);
    setOpacity(1.0);
    setFlags(QGraphicsItem::ItemIsSelectable
           | QGraphicsItem::ItemIsMovable
           | QGraphicsItem::ItemIsFocusable
           | QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

void RectItem::setRectFromPoints(const QPointF &start, const QPointF &end)
{
    QRectF r(start, end);
    r = r.normalized();
    setRect(r);
}

QVector<QPointF> RectItem::getCornerPoints() const
{
    QRectF r = rect();
    return {r.topLeft(), r.topRight(), r.bottomRight(), r.bottomLeft()};
}

QRectF RectItem::boundingRect() const
{
    qreal extra = pen().widthF() / 2.0 + m_handleSize;
    return rect().adjusted(-extra, -extra, extra, extra);
}

QPainterPath RectItem::shape() const
{
    QPainterPath path;
    QRectF r = rect();
    qreal w = pen().widthF();
    qreal h = m_handleSize * 2;
    path.addRect(r.adjusted(-w - h, -w - h, w + h, w + h));
    return path;
}

void RectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(pen());
    painter->setBrush(brush());
    painter->setOpacity(opacity());
    painter->drawRect(rect());

    if (isSelected() || hasFocus()) {
        painter->setBrush(Qt::white);
        painter->setPen(QPen(Qt::black, 1));
        QRectF r = rect();
        qreal hs = m_handleSize / 2.0;
        painter->drawEllipse(r.topLeft(), hs, hs);
        painter->drawEllipse(r.topRight(), hs, hs);
        painter->drawEllipse(r.bottomLeft(), hs, hs);
        painter->drawEllipse(r.bottomRight(), hs, hs);
        painter->drawEllipse(QPointF(r.left() + r.width() / 2, r.top()), hs, hs);
        painter->drawEllipse(QPointF(r.left() + r.width() / 2, r.bottom()), hs, hs);
        painter->drawEllipse(QPointF(r.left(), r.top() + r.height() / 2), hs, hs);
        painter->drawEllipse(QPointF(r.right(), r.top() + r.height() / 2), hs, hs);
    }
}

RectItem::Handle RectItem::handleAt(const QPointF &pos) const
{
    QRectF r = rect();
    qreal d = m_handleSize * 2;

    if (QRectF(r.topLeft() - QPointF(d, d), QSizeF(d * 2, d * 2)).contains(pos)) return TopLeft;
    if (QRectF(r.topRight() - QPointF(d, d), QSizeF(d * 2, d * 2)).contains(pos)) return TopRight;
    if (QRectF(r.bottomLeft() - QPointF(d, d), QSizeF(d * 2, d * 2)).contains(pos)) return BottomLeft;
    if (QRectF(r.bottomRight() - QPointF(d, d), QSizeF(d * 2, d * 2)).contains(pos)) return BottomRight;

    QRectF topEdge(r.left() + d, r.top() - d, r.width() - d * 2, d * 2);
    QRectF bottomEdge(r.left() + d, r.bottom() - d, r.width() - d * 2, d * 2);
    QRectF leftEdge(r.left() - d, r.top() + d, d * 2, r.height() - d * 2);
    QRectF rightEdge(r.right() - d, r.top() + d, d * 2, r.height() - d * 2);

    if (topEdge.contains(pos)) return Top;
    if (bottomEdge.contains(pos)) return Bottom;
    if (leftEdge.contains(pos)) return Left;
    if (rightEdge.contains(pos)) return Right;

    if (rect().contains(pos)) return Interior;

    return None;
}

void RectItem::updateCursor(Handle h)
{
    switch (h) {
    case TopLeft: case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
    case Top: case Bottom: setCursor(Qt::SizeVerCursor); break;
    case Left: case Right: setCursor(Qt::SizeHorCursor); break;
    case Interior: setCursor(Qt::SizeAllCursor); break;
    default: setCursor(Qt::ArrowCursor); break;
    }
}

void RectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isSelected()) {
        updateCursor(handleAt(event->pos()));
    }
}

void RectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
}

void RectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_activeHandle = handleAt(event->pos());
        if (m_activeHandle != None && m_activeHandle != Interior) {
            setSelected(true);
            setFocus(Qt::MouseFocusReason);
            m_pressPos = event->pos();
            m_pressRect = rect();
            event->accept();
            return;
        }
    }
    QGraphicsRectItem::mousePressEvent(event);
}

QRectF RectItem::clampRectToBoundary(const QRectF &r) const
{
    if (m_boundary.isNull())
        return r;
    QRectF result = r;
    if (result.left() < m_boundary.left())
        result.moveLeft(m_boundary.left());
    if (result.top() < m_boundary.top())
        result.moveTop(m_boundary.top());
    if (result.right() > m_boundary.right())
        result.moveRight(m_boundary.right());
    if (result.bottom() > m_boundary.bottom())
        result.moveBottom(m_boundary.bottom());
    return result;
}

void RectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle == None || m_activeHandle == Interior) {
        QGraphicsRectItem::mouseMoveEvent(event);
        return;
    }

    QPointF delta = event->pos() - m_pressPos;
    QRectF r = m_pressRect;

    switch (m_activeHandle) {
    case TopLeft:     r.setTopLeft(r.topLeft() + delta); break;
    case TopRight:    r.setTopRight(r.topRight() + delta); break;
    case BottomLeft:  r.setBottomLeft(r.bottomLeft() + delta); break;
    case BottomRight: r.setBottomRight(r.bottomRight() + delta); break;
    case Top:         r.setTop(r.top() + delta.y()); break;
    case Bottom:      r.setBottom(r.bottom() + delta.y()); break;
    case Left:        r.setLeft(r.left() + delta.x()); break;
    case Right:       r.setRight(r.right() + delta.x()); break;
    default: break;
    }

    r = clampRectToBoundary(r.normalized());
    if (r.width() > 2 && r.height() > 2)
        setRect(r);

    event->accept();
}

void RectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_activeHandle = None;
    QGraphicsRectItem::mouseReleaseEvent(event);
}

QVariant RectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && !m_boundary.isNull()) {
        QPointF newPos = value.toPointF();
        QRectF r = rect();
        QRectF newRect(newPos.x() + r.left(), newPos.y() + r.top(), r.width(), r.height());
        newRect = clampRectToBoundary(newRect);
        QPointF clampedPos(newRect.left() - r.left(), newRect.top() - r.top());
        return clampedPos;
    }
    return QGraphicsRectItem::itemChange(change, value);
}
