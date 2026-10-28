#ifndef VISIBILITYMANAGER_H
#define VISIBILITYMANAGER_H

class QGraphicsScene;

class VisibilityManager
{
public:
    explicit VisibilityManager(QGraphicsScene *scene);

    void setInferPatternVisible(bool visible);
    void setPaintedItemVisible(bool visible);
    void setRectItemVisible(bool visible);

private:
    QGraphicsScene *m_scene;
};

#endif // VISIBILITYMANAGER_H
