#include "visibilitymanager.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include "inferpattern.h"
#include "painteditem.h"

VisibilityManager::VisibilityManager(QGraphicsScene *scene)
    : m_scene(scene)
{
}

void VisibilityManager::setInferPatternVisible(bool visible)
{
    if (!m_scene) return;
    const auto &items = m_scene->items();
    for (QGraphicsItem *item : items)
    {
        InferPattern *pattern = dynamic_cast<InferPattern *>(item);
        if (pattern)
            pattern->setVisible(visible);
    }
}

void VisibilityManager::setPaintedItemVisible(bool visible)
{
    if (!m_scene) return;
    const auto &items = m_scene->items();
    for (QGraphicsItem *item : items)
    {
        PaintedItem *pattern = dynamic_cast<PaintedItem *>(item);
        if (pattern)
            pattern->setVisible(visible);
    }
}
