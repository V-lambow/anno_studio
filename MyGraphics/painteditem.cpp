#include "painteditem.h"
#include <math.h>

PaintedItem::PaintedItem(QColor penColor, int penSize, QGraphicsPathItem *parent)
    : QGraphicsPathItem(parent)
{
    setPen(QPen(penColor,penSize));
    setFlag(QGraphicsPathItem::ItemIsSelectable, true);
    setFlag(QGraphicsPathItem::ItemIsFocusable, true);
}

QRectF PaintedItem::boundingRect() const
{

    qreal margin = pen().width() / 2.0 + 2; // 预留线宽+抗锯齿冗余
    return QGraphicsPathItem::boundingRect().adjusted(-margin, -margin, margin, margin);

}
QPainterPath PaintedItem::shape() const {
    QPainterPath path = QGraphicsPathItem::shape();
    QPainterPathStroker stroker;
    stroker.setWidth(pen().width()); // 匹配画笔宽度
    return stroker.createStroke(path); // 扩展碰撞区域
}

void PaintedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

StrokeItem::StrokeItem(QColor color, int penSize, int classId, QGraphicsPathItem *parent)
    : PaintedItem(color, penSize, parent)
{
    m_classId=classId;
    QPen p = pen();
    p.setCapStyle(Qt::RoundCap);
    p.setJoinStyle(Qt::RoundJoin);
    QColor c = p.color();
    c.setAlpha(51);
    p.setColor(c);
    setPen(p);
}

void StrokeItem::addPoint(const QPointF &point,bool isPress)
{
    QPointF localPoint = mapFromScene(point);
    if (isPress) {
        m_path.moveTo(localPoint);
    } else {
        m_path.lineTo(localPoint);
    }
    setPath(m_path);
    update();
}



bool StrokeItem::eraseAt(const QPointF &scenePos, qreal radius)
{
    if (m_isPolygon) {
        QPointF localPos = mapFromScene(scenePos);
        QPainterPath eraser;
        eraser.addEllipse(localPos, radius, radius);
        m_path = m_path.subtracted(eraser);
        m_path.setFillRule(Qt::OddEvenFill);
        setPath(m_path);
        update();
        return !m_path.isEmpty();
    }

    QPointF localPos = mapFromScene(scenePos);
    QPainterPath result;

    auto inCircle = [&](const QPointF &p) {
        return QLineF(p, localPos).length() <= radius;
    };

    auto intersect = [&](const QPointF &from, const QPointF &to) -> QPointF {
        QPointF d = to - from;
        QPointF ac = from - localPos;
        qreal a = d.x()*d.x() + d.y()*d.y();
        if (a < 1e-10) return to;
        qreal b = 2 * (d.x()*ac.x() + d.y()*ac.y());
        qreal c = ac.x()*ac.x() + ac.y()*ac.y() - radius*radius;
        qreal disc = b*b - 4*a*c;
        if (disc < 0) return to;
        qreal t = (-b - sqrt(disc)) / (2*a);
        if (t < 0 || t > 1) t = (-b + sqrt(disc)) / (2*a);
        if (t < 0 || t > 1) return to;
        return from + t * d;
    };

    int n = m_path.elementCount();
    for (int i = 0; i < n; ) {
        if (!m_path.elementAt(i).isMoveTo()) { i++; continue; }

        QPointF movePt(m_path.elementAt(i).x, m_path.elementAt(i).y);
        bool prevIn = inCircle(movePt);
        bool building = !prevIn;
        QPointF lastGoodPt = movePt;

        if (building)
            result.moveTo(movePt);

        int j = i + 1;
        while (j < n && m_path.elementAt(j).isLineTo()) {
            QPointF curr(m_path.elementAt(j).x, m_path.elementAt(j).y);
            bool currIn = inCircle(curr);

            if (currIn && prevIn) {
                // skip
            } else if (!currIn && !prevIn) {
                result.lineTo(curr);
                lastGoodPt = curr;
            } else if (currIn && !prevIn) {
                QPointF bp = intersect(lastGoodPt, curr);
                result.lineTo(bp);
                building = false;
                lastGoodPt = bp;
            } else if (!currIn && prevIn) {
                QPointF fromPt = (j > i + 1)
                    ? QPointF(m_path.elementAt(j-1).x, m_path.elementAt(j-1).y)
                    : movePt;
                QPointF bp = intersect(fromPt, curr);
                if (!building) {
                    result.moveTo(bp);
                    building = true;
                } else {
                    result.lineTo(bp);
                }
                result.lineTo(curr);
                lastGoodPt = curr;
            }

            prevIn = currIn;
            j++;
        }
        i = j;
    }

    m_path = result;
    setPath(m_path);
    update();
    return true;
}

void StrokeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    if (m_isPolygon) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(pen().color());
    } else {
        painter->setPen(pen());
    }
    painter->drawPath(m_path);

    if (isSelected()) {
        QPen selPen(QColor(0, 120, 215), pen().widthF() + 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        selPen.setColor(QColor(0, 120, 215, 180));
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_path);
    }

    painter->restore();
}


// 获取笔迹外围的所有点
QVector<QVector<QPointF>> StrokeItem::getOutlinePoints() const
{
    QVector<QVector<QPointF>> outlinePoints;
    if (m_path.isEmpty()) return outlinePoints;

    QPainterPath src = m_path;
    if (!m_isPolygon) {
        QPainterPathStroker stroker;
        stroker.setWidth(pen().widthF());
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);
        src = stroker.createStroke(m_path);
    }

    auto polygons = src.toFillPolygons(QTransform());
    outlinePoints.reserve(polygons.size());
    for (const auto& poly : polygons)
    {
        QVector<QPointF> polyF{poly.cbegin(), poly.cend()};
        outlinePoints.push_back(std::move(polyF));
    }
    return outlinePoints;
}
QPainterPath StrokeItem::shape() const
{
    if (m_isPolygon)
        return m_path;
    return PaintedItem::shape();
}

void StrokeItem::setStrokePath(const QPainterPath &path)
{
    m_path = path;
    setPath(m_path);
    update();
}

void StrokeItem::setPolygonFromPoints(const QVector<QPointF>& points)
{
    if (points.isEmpty()) return;
    m_path = QPainterPath();
    m_path.moveTo(points[0]);
    for (int i = 1; i < points.size(); ++i)
        m_path.lineTo(points[i]);
    m_path.closeSubpath();
    setPath(m_path);
    m_isPolygon = true;
    update();
}

int StrokeItem::setClassId(int classId){
    m_classId = classId;
    return m_classId;
}
QRectF StrokeItem::boundingRect() const {
    return PaintedItem::boundingRect();
}
