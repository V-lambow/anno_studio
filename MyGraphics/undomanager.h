#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <QList>
#include <QPair>
#include "painteditem.h"
#include "rectitem.h"

struct UndoSnapshot {
    QList<StrokeItem*> strokes;
    QList<RectItem*> rects;
};

class UndoManager
{
public:
    explicit UndoManager(int maxHistory = 20);
    ~UndoManager();

    void store(const QList<StrokeItem*> &strokes, const QList<RectItem*> &rects);
    bool canUndo() const;
    UndoSnapshot undo();

    void clear();
    void trimTo(int maxHistory);

private:
    QList<UndoSnapshot> m_history;
    int m_maxHistory;
};

#endif // UNDOMANAGER_H
