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

StrokeItem::StrokeItem(Qt::GlobalColor emColor, int penSize, QGraphicsPathItem *parent)
    : PaintedItem(QColor(emColor), penSize, parent)
{
    m_classId=setClassId(emColor);
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



void StrokeItem::eraseAt(const QPointF &scenePos, qreal radius)
{
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
}

void StrokeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setPen(pen());
    painter->drawPath(m_path);
    painter->restore();

}


// 获取笔迹外围的所有点
QVector<QVector<QPointF>> StrokeItem::getOutlinePoints() const
{
    QVector<QVector<QPointF>> outlinePoints;
    if (m_path.isEmpty()) return outlinePoints;

    auto polygons =m_path.toFillPolygons(QTransform());
    outlinePoints.reserve(polygons.size());
    for (const auto& poly : polygons)
    {
        // 直接通过初始化列表转换 QPolygon → QVector<QPointF>
        QVector<QPointF> polyF{poly.cbegin(), poly.cend()};
        outlinePoints.push_back(std::move(polyF)); // 移动语义，避免拷贝
    }
    return outlinePoints;
}
QVector<QRectF> StrokeItem::getBboxesF(const QVector<QVector<QPointF>>& pointSets) const
{
    QVector<QRectF> bboxes;
    bboxes.reserve(pointSets.size()); // 预分配内存，提升性能

    // 遍历每个点集（每个内层 QVector<QPointF>）
    for (const QVector<QPointF>& points : pointSets)
    {
        // 1. 处理空点集：跳过（或构造空矩形，按需调整）
        if (points.isEmpty())
        {
            continue; // 若需要保留空矩形，可替换为 bboxes.append(QRectF());
        }

        // 2. 初始化极值（用浮点极值避免漏判）
        qreal minX = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::lowest();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxY = std::numeric_limits<qreal>::lowest();

        // 3. 遍历点集，更新 x/y 极值
        for (const QPointF& p : points)
        {
            minX = qMin(minX, p.x());
            maxX = qMax(maxX, p.x());
            minY = qMin(minY, p.y());
            maxY = qMax(maxY, p.y());
        }

        // 4. 构造水平包围框（QRectF(minX, minY, 宽度, 高度)）
        QRectF bbox(minX, minY, maxX - minX, maxY - minY);

        // 5. （可选）过滤极小无效框（比如单点导致的 0 宽/高框）
        const qreal minValidSize = 1e-6; // 浮点精度阈值
        if (bbox.width() < minValidSize && bbox.height() < minValidSize)
        {
            continue;
        }

        // 6. 添加到结果列表
        bboxes.append(bbox);
    }

    return bboxes;
}

QVector<QVector<QPointF>> StrokeItem::getPathPoints() const
{
    QVector<QVector<QPointF>> segments;
    int n = m_path.elementCount();
    int i = 0;
    while (i < n) {
        if (!m_path.elementAt(i).isMoveTo()) { i++; continue; }
        QVector<QPointF> seg;
        seg.append(QPointF(m_path.elementAt(i).x, m_path.elementAt(i).y));
        i++;
        while (i < n && m_path.elementAt(i).isLineTo()) {
            seg.append(QPointF(m_path.elementAt(i).x, m_path.elementAt(i).y));
            i++;
        }
        if (seg.size() >= 2)
            segments.append(seg);
    }
    return segments;
}

void StrokeItem::setPathFromPoints(const QVector<QVector<QPointF>>& segments)
{
    m_path = QPainterPath();
    for (const auto &seg : segments) {
        if (seg.isEmpty()) continue;
        m_path.moveTo(seg[0]);
        for (int i = 1; i < seg.size(); ++i)
            m_path.lineTo(seg[i]);
    }
    setPath(m_path);
    update();
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
    update();
}

int StrokeItem::setClassId(Qt::GlobalColor color){
    return qcolor2labelIdx.at(static_cast<int>(color));
}
QRectF StrokeItem::boundingRect() const {
    return PaintedItem::boundingRect();
}
