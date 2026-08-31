#include "mygraphicsview.h"
#include <QScrollBar>


// MyGraphicsView 实现
MyGraphicsView::MyGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
    , m_penWidth(10)
    , m_penColor(Qt::red)
{
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setMouseTracking(true);
    updateCursor();
}
int MyGraphicsView::getFactor()
{
    return factor_;

}

void MyGraphicsView::setFactor(int factor)
{
    if (factor == 0)
    {
        factor_ = 0;
    }
    else
    {
        factor_ += factor;
    }
    viewport()->update();
}

void MyGraphicsView::wheelEvent(QWheelEvent *e)
{
    if (!(e->modifiers() & Qt::ControlModifier)) {
        QGraphicsView::wheelEvent(e);
        return;
    }

    const double factor = 1.15;
    double direction = (e->angleDelta().y() > 0) ? factor : 1.0 / factor;

    QPointF cursorPos = e->position();
    QPointF sceneBefore = mapToScene(cursorPos.toPoint());

    scale(direction, direction);

    QPointF sceneAfter = mapToScene(cursorPos.toPoint());
    QPointF delta = sceneAfter - sceneBefore;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - int(delta.x()));
    verticalScrollBar()->setValue(verticalScrollBar()->value() - int(delta.y()));

    e->accept();
}

void MyGraphicsView::Translate(QPointF delta){
    // 获取当前视图的中心位置
    QPointF currentCenter = mapToScene(viewport()->rect().center());

    // 计算新的中心位置 - 注意方向
    QPointF newCenter = currentCenter + delta;

    // 将视图中心设置为新的位置
    centerOn(newCenter);
}


void MyGraphicsView::mousePressEvent(QMouseEvent *e)
{

    if (e->button() == Qt::RightButton) {
        if (m_cursorMode == CURSOR_MODE::create_roi) {
            m_cursorMode = CURSOR_MODE::origin;
            m_roiDrawing = false;
            m_roiPreviewRect = QRectF();
            viewport()->update();
            return;
        }
        this->m_isDragging = true;
        this->m_DraggingPos = e->pos(); // 保存视图坐标，不是场景坐标
        // setCursor(Qt::ClosedHandCursor);
        e->accept(); // 明确接受事件
        return; // 不要调用基类函数，避免重复处理
    }

    //左键功能
    QPointF scenePos = mapToScene(e->pos());
    
    switch (this->m_cursorMode) {
    case CURSOR_MODE::eraser: {
        if (!m_drawEffectEnabled) break;
        doErase(scenePos);
        m_isErasing = true;
        e->accept();
        return;
    }
    case CURSOR_MODE::pen_brush:{
        if (!m_drawEffectEnabled) break;
        StrokeItem *target = new StrokeItem(m_penColor, m_penWidth, m_currentClassId);
        m_strokeItems.append(target);
        scene()->addItem(target);
        m_currentStroke = target;
        m_currentStroke->addPoint(scenePos, true);
        m_isDrawing = true;
        m_currentStroke->setZValue(0);
        return;
    }
    case CURSOR_MODE::create_roi:{
        m_roiStartPos = scenePos;
        m_roiDrawing = true;
        m_roiPreviewRect = QRectF(scenePos, scenePos);
        e->accept();
        return;
    }
    case CURSOR_MODE::create_rect:{
        if (!m_drawEffectEnabled) break;
        m_rectStartPos = scenePos;
        RectItem *rect = new RectItem(m_penColor, m_penWidth, m_currentClassId);
        m_rectItems.append(rect);
        scene()->addItem(rect);
        m_currentRect = rect;
        m_currentRect->setRectFromPoints(scenePos, scenePos);
        m_isDrawing = true;
        m_currentRect->setZValue(0);
        e->accept();
        return;
    }
    default:
        break;
    }
    
    m_lastMousePos = scenePos;
    QGraphicsView::mousePressEvent(e);
}

void MyGraphicsView::mouseMoveEvent(QMouseEvent *e)
{
    QPointF scenePos = mapToScene(e->pos());
    emit this->sg_mouseMoved(scenePos);
    
    if (m_cursorMode == CURSOR_MODE::create_roi && m_roiDrawing) {
        m_roiPreviewRect = QRectF(m_roiStartPos, scenePos).normalized();
        viewport()->update();
        return;
    }

    if (m_cursorMode == CURSOR_MODE::create_rect && m_isDrawing && m_currentRect) {
        m_currentRect->setRectFromPoints(m_rectStartPos, scenePos);
        viewport()->update();
        return;
    }

    ///右键拖拽功能
    if (this->m_isDragging && (e->buttons() & Qt::RightButton)) {
        // 计算移动的距离（使用视图坐标差值，然后转换为场景距离）
        QPointF lastScenePos = mapToScene(m_DraggingPos);
        QPointF currentScenePos = mapToScene(e->pos());
        QPointF delta = currentScenePos - lastScenePos;

        // 应用平移
        Translate(-delta); // 注意：这里使用负值，因为移动方向与鼠标方向相反

        // 更新拖拽位置
        this->m_DraggingPos = e->pos();
        e->accept();
        return; // 不要调用基类函数，避免冲突
    }
    m_lastMousePos = scenePos;
    if (m_isDrawing && m_currentStroke) {
    m_currentStroke->addPoint(scenePos,false);
    }
    if (m_isErasing && (e->buttons() & Qt::LeftButton)) {
        doErase(scenePos);
    }
    viewport()->update();
    QGraphicsView::mouseMoveEvent(e);
}

void MyGraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
    // 右键鼠标释放
    if (e->button() == Qt::RightButton && this->m_isDragging) {
        this->m_isDragging = false;
        setCursor(Qt::CrossCursor);
        e->accept();
        return; // 不要调用基类函数，避免冲突
    }
    if (e->button() == Qt::LeftButton) {
        if (m_cursorMode == CURSOR_MODE::create_roi && m_roiDrawing) {
            m_roiDrawing = false;
            QRectF roiRect = m_roiPreviewRect;
            m_roiPreviewRect = QRectF();
            m_cursorMode = CURSOR_MODE::origin;
            setCursor(Qt::ArrowCursor);
            viewport()->update();
            if (roiRect.width() > 5 && roiRect.height() > 5) {
                emit sg_roiDrawn(roiRect);
            }
            return;
        }
        if (m_cursorMode == CURSOR_MODE::create_rect && m_isDrawing) {
            m_currentRect = nullptr;
            m_isDrawing = false;
            emit sg_drawingFinished();
            e->accept();
            return;
        }
        if (m_isDrawing) {
            m_currentStroke = nullptr;
            m_isDrawing = false;
            emit sg_drawingFinished();
        }
        if (m_isErasing) {
            m_isErasing = false;
            emit sg_drawingFinished();
        }
        if (m_isDrawing || m_isErasing) {
            e->accept();
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(e);
    updateCursor();
}

void MyGraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);
    if (m_cursorMode == CURSOR_MODE::eraser) {
        painter->setBrush(QBrush(Qt::white, Qt::Dense4Pattern));
        painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
        painter->drawEllipse(m_lastMousePos, m_penWidth, m_penWidth);
    }
    if (m_cursorMode == CURSOR_MODE::create_roi && !m_roiPreviewRect.isNull()) {
        painter->setPen(QPen(Qt::black, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(m_roiPreviewRect);
    }
}

void MyGraphicsView::doErase(const QPointF &scenePos)
{
    qreal r = m_penWidth;
    QPainterPath eraserShape;
    eraserShape.addEllipse(scenePos, r, r);
    auto items = scene()->items(eraserShape, Qt::IntersectsItemShape);
    for (auto *item : items) {
        auto *stroke = dynamic_cast<StrokeItem*>(item);
        if (stroke) {
            if (!stroke->eraseAt(scenePos, r)) {
                scene()->removeItem(stroke);
                removeStroke(stroke);
                emit sg_itemErasedCompletely(stroke);
                delete stroke;
            }
        }
    }
}

void MyGraphicsView::updateCursor()
{
    Qt::CursorShape shape;
    switch (m_cursorMode) {
    case CURSOR_MODE::pen_brush:
    case CURSOR_MODE::eraser:
    case CURSOR_MODE::create_roi:
    case CURSOR_MODE::create_rect:
        shape = Qt::CrossCursor;
        break;
    default:
        shape = Qt::ArrowCursor;
        break;
    }
    setCursor(shape);
    viewport()->setCursor(shape);
}

QList<QPair<int, QVector<QVector<QPointF>>>> MyGraphicsView::getAllStrokesOutlineData() const
{
    QList<QPair<int, QVector<QVector<QPointF>>>> result;
    for (const auto* stroke : m_strokeItems) {
        auto outlines = stroke->getOutlinePoints();
        for (auto& poly : outlines) {
            for (auto& pt : poly) {
                pt = stroke->mapToScene(pt);
            }
        }
        result.append({stroke->classId(), std::move(outlines)});
    }
    return result;
}

void MyGraphicsView::clearStrokes()
{
    for (auto* stroke : m_strokeItems) {
        scene()->removeItem(stroke);
        delete stroke;
    }
    m_strokeItems.clear();
    m_currentStroke = nullptr;
    m_isDrawing = false;
    m_isErasing = false;
}

void MyGraphicsView::removeStroke(StrokeItem *stroke)
{
    m_strokeItems.removeOne(stroke);
}

QList<QPair<int, QVector<QPointF>>> MyGraphicsView::getAllRectData() const
{
    QList<QPair<int, QVector<QPointF>>> result;
    for (const auto* rect : m_rectItems) {
        QVector<QPointF> points;
        QRectF r = rect->rect();
        points.append(rect->mapToScene(r.topLeft()));
        points.append(rect->mapToScene(r.topRight()));
        points.append(rect->mapToScene(r.bottomRight()));
        points.append(rect->mapToScene(r.bottomLeft()));
        result.append({rect->classId(), std::move(points)});
    }
    return result;
}

void MyGraphicsView::clearRects()
{
    for (auto* rect : m_rectItems) {
        scene()->removeItem(rect);
        delete rect;
    }
    m_rectItems.clear();
    m_currentRect = nullptr;
}

void MyGraphicsView::removeRect(RectItem *rect)
{
    m_rectItems.removeOne(rect);
}

void MyGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        const auto selected = scene()->selectedItems();
        for (QGraphicsItem *item : selected)
        {
            if (StrokeItem *stroke = qgraphicsitem_cast<StrokeItem*>(item))
            {
                m_strokeItems.removeOne(stroke);
                scene()->removeItem(stroke);
                delete stroke;
            }
            else if (RectItem *rect = qgraphicsitem_cast<RectItem*>(item))
            {
                m_rectItems.removeOne(rect);
                scene()->removeItem(rect);
                delete rect;
            }
        }
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

