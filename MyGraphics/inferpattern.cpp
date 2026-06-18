#include "inferpattern.h"

InferPattern::InferPattern(Labelele labelele, QColor color, QGraphicsItem *parent)
    : QGraphicsItem(parent), labelele_(std::move(labelele)), color_(color)
{
    // 设置项目可接收悬停事件
    setAcceptHoverEvents(true);

    // 计算中心和面积
    getCenter();
    getArea();

    // 设置项目不可选择和移动
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);

}


QRectF InferPattern::boundingRect() const
{
    // 返回包围盒，适当扩大一点以确保绘制完整
    return labelele_.bbox.adjusted(-2, -2, 2, 2);
}

void InferPattern::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 保存当前画笔状态
    painter->save();

    // 设置画笔和画刷
    color_.setAlpha(100);
    painter->setPen(QPen(color_, 2));
    painter->setBrush(QBrush(color_, Qt::SolidPattern));

    // 如果有足够的点，绘制多边形
    if (labelele_.pts.size() >= 2) {
        QPolygonF polygon;
        for (const auto& point : labelele_.pts) {
            polygon.append(point);
        }
        painter->drawPolygon(polygon);
    }
    // 如果只有一个点，绘制圆点
    else if (labelele_.pts.size() == 1) {
        painter->drawEllipse(labelele_.pts[0], 5, 5);
    }

    // 恢复画笔状态
    painter->restore();
}

void InferPattern::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItem::hoverEnterEvent(event);
    // 鼠标进入时显示提示信息
    hoverMoveEvent(event);
}

void InferPattern::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItem::hoverLeaveEvent(event);
    // 鼠标离开时隐藏提示
    QToolTip::hideText();
}

void InferPattern::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsItem::hoverMoveEvent(event);

    // 构建提示信息
    QString tooltip = QString("标签: %1\n")
                          .arg(labelele_.labelName ? labelele_.labelName : "未知");
    tooltip += QString("中心: (%1, %2)\n")
                   .arg(center_.x(), 0, 'f', 1)
                   .arg(center_.y(), 0, 'f', 1);
    tooltip += QString("面积: %1").arg(area_);
    // 置信度分数
    tooltip += QString("\n置信度: %1").arg(labelele_.score, 0, 'f', 2);
    // 最大外接矩阵 整数坐标
    tooltip += QString("\n最大外接矩阵: (%1 ,%2):(%3 ,%4)")
                   .arg(labelele_.bbox.topLeft().x(), 0, 'f', 0)
                   .arg(labelele_.bbox.topLeft().y(), 0, 'f', 0)
                   .arg(labelele_.bbox.bottomRight().x(), 0, 'f', 0)
                   .arg(labelele_.bbox.bottomRight().y(), 0, 'f', 0);

    // 在鼠标位置显示提示
    QToolTip::showText(event->screenPos(), tooltip);
}

void InferPattern::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 忽略鼠标按下事件，确保不能拖动
    event->ignore();
}

void InferPattern::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // 忽略鼠标移动事件，确保不能拖动
    event->ignore();
}

void InferPattern::getCenter()
{
    // 计算多边形中心（包围盒中心）
    center_ = labelele_.bbox.center();
}

void InferPattern::getArea()
{
    // 计算多边形面积（使用包围盒的面积）
    area_ = labelele_.area;
}


