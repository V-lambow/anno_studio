#include "mygraphicsview.h"


// MyGraphicsView 实现
MyGraphicsView::MyGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
    , m_penWidth(10)
    , m_penEMColor(Qt::red)
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
    // Calculate wheel movement
    int distance = e->angleDelta().y()/15/8;
    qreal val;

    if (distance != 0)
    {
        val = pow(1.2, distance);
        this->scale(val, val);
        if (distance > 0)
        {
            factor_ ++;
        }
        else
        {
            factor_ --;
        }
    }
    // viewport()->update();
    // if(m_currentStroke){
    // this->m_currentStroke->setZValue(0);
    // }

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
        doErase(scenePos);
        m_isErasing = true;
        e->accept();
        return;
    }
    case  CURSOR_MODE::pen_brush:{
        StrokeItem *target = nullptr;
        if (!m_strokeItems.isEmpty()) {
            StrokeItem *last = m_strokeItems.last();
            if (last->pen().color().rgb() == QColor(m_penEMColor).rgb())
                target = last;
        }
        if (!target) {
            target = new StrokeItem(m_penEMColor, m_penWidth);
            m_strokeItems.append(target);
            scene()->addItem(target);
        }
        m_currentStroke = target;
        m_currentStroke->addPoint(scenePos, true);
        m_isDrawing = true;
        m_currentStroke->setZValue(0);
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
        if (m_isDrawing) {
            m_currentStroke = nullptr;
            m_isDrawing = false;
        }
        if (m_isErasing) {
            m_isErasing = false;
        }
        if (m_isDrawing || m_isErasing) {
            e->accept();
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(e);
}

void MyGraphicsView::drawForeground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);
    if (m_cursorMode == CURSOR_MODE::eraser) {
        painter->setBrush(QBrush(Qt::white, Qt::Dense4Pattern));
        painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
        painter->drawEllipse(m_lastMousePos, m_penWidth, m_penWidth);
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
            stroke->eraseAt(scenePos, r);
        }
    }
}

void MyGraphicsView::updateCursor()
{
    switch (m_cursorMode) {
    case CURSOR_MODE::pen_brush:
    case CURSOR_MODE::eraser:
        setCursor(Qt::CrossCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

