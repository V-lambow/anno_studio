#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include "mygraphicsitem.h"


class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MyGraphicsView(QWidget *parent = nullptr);

    void setCurrentTool(int tool) { m_currentTool = tool; }
    void setPenWidth(int width) { m_penWidth = width; }
    void setPenColor(QColor color) { m_penColor = color; }
    void updateCursor();

signals:
    void itemHovered(MyGraphicsItem *item);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    int m_currentTool; // 0: cursor, 1: circle, 2: square, 3: eraser
    int m_penWidth;
    QColor m_penColor;
    QPointF m_lastMousePos;
};
#endif // MYGRAPHICSVIEW_H
