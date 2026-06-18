#ifndef PAINTEDITEM_H
#define PAINTEDITEM_H

#include <QGraphicsPathItem>
#include <QCursor>
#include <QPixmap>
#include <QPainter>
#include <QStatusBar>
#include <QPointF>
#include <QRectF>
#include "../include/base_head.hpp"

extern const std::unordered_map<int,int> qcolor2labelIdx;


// 自定义图形项基类
class PaintedItem : public QGraphicsPathItem
{
public:
    enum { Type = UserType + 1 };

    PaintedItem(QColor penColor, int penSize, QGraphicsPathItem *parent = nullptr);

    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    virtual int type() const override { return Type; }

};

class StrokeItem : public PaintedItem
{
public:
    StrokeItem(Qt::GlobalColor emColor, int penSize, QGraphicsPathItem *parent = nullptr);

    // 添加新的点到路径
    void addPoint(const QPointF &point,bool isPress);
    void eraseAt(const QPointF &scenePos, qreal radius);

    void pathClear(){m_path.clear();}

    // 获取笔迹外围的所有点
    QVector<QVector<QPointF>> getOutlinePoints() const;
    QVector<QRectF> getBboxesF(const QVector<QVector<QPointF>>& pointSets) const;
    QRectF boundingRect() const override;


protected:
    // 重写绘制函数
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    int setClassId(Qt::GlobalColor color);

private:
    QPainterPath m_path;
    int m_classId=0;
};


#endif // PAINTEDITEM_H
