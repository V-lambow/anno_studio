#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include "painteditem.h"
#include "rectitem.h"
#include "inferpattern.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QPoint>
#include <QPair>
#include "../include/base_head.hpp"


class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MyGraphicsView(QWidget *parent = nullptr);

    void setPenWidth(int width) { m_penWidth = width; }
    void setPenColor(QColor color) { m_penColor = color; }
    void setCurrentClassId(int id) { m_currentClassId = id; }
    void updateCursor();
    int getFactor();
    void setFactor(int _factor);
    void resetCursorMode(){m_cursorMode=CURSOR_MODE::origin;};
    void switchCursorMode(CURSOR_MODE mode){m_cursorMode=mode; updateCursor();};
    void setDrawEffectEnabled(bool enabled) { m_drawEffectEnabled = enabled; }
    QList<QPair<int, QVector<QVector<QPointF>>>> getAllStrokesOutlineData() const;
    void clearStrokes();
    void removeStroke(StrokeItem *stroke);

    // 矩形相关方法
    QList<QPair<int, QVector<QPointF>>> getAllRectData() const;
    void clearRects();
    void removeRect(RectItem *rect);

signals:
    void sg_mouseMoved(const QPointF& pt);
    void sg_drawingFinished();
    void sg_itemErasedCompletely(StrokeItem *item);
    void sg_roiDrawn(const QRectF &rect);

    // void itemHovered(MyGraphicsItem *item);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void Translate(QPointF delta);
    virtual void wheelEvent(QWheelEvent *e) override;
    QVector<QVector<QPointF>> getAllStrokeOutlinePoints() const;


private:
    void doErase(const QPointF &scenePos);
    ///放大视图
    int factor_;

    ///右键拖拽画图
    bool m_isDragging =false;
    QPoint m_DraggingPos{};


    CURSOR_MODE m_cursorMode =CURSOR_MODE::origin;
    int m_penWidth;
    QColor m_penColor;
    int m_currentClassId = 0;
    bool m_drawEffectEnabled = true;
    QPointF m_lastMousePos;
    // StrokeItem *m_currentStroke=nullptr;
    bool m_isDrawing=false;
    bool m_isErasing=false;
    QList<StrokeItem*> m_strokeItems; // 存储所有笔迹项
    StrokeItem* m_currentStroke =nullptr;

    // 矩形绘制
    QPointF m_rectStartPos;
    RectItem* m_currentRect = nullptr;
    QList<RectItem*> m_rectItems;

    // ROI绘制
    QPointF m_roiStartPos;
    QRectF m_roiPreviewRect;
    bool m_roiDrawing = false;

};
#endif // MYGRAPHICSVIEW_H
