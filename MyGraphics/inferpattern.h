#ifndef INFERPATTERN_H
#define INFERPATTERN_H

#include <QObject>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QPixmap>
#include <QPainter>
#include <QStatusBar>
#include <QPointF>
#include <QRectF>
#include <QToolTip>
#include "../include/base_head.hpp"

class InferPattern : public QGraphicsItem
{
    // Q_OBJECT
public:

    InferPattern(Labelele labelele, QColor color, QGraphicsItem *parent = nullptr);
    InferPattern(Labelele labelele, Qt::GlobalColor emColor, QGraphicsItem *parent = nullptr):\
        InferPattern(labelele,QColor(emColor),parent){}

    // essential override
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    // override
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

    // delete
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Labelele labelele_;
    QColor color_;
    QPointF center_;
    int area_{0};
    void getCenter();
    void getArea();

signals:
};

#endif // INFERPATTERN_H
