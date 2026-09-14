#include "undomanager.h"

UndoManager::UndoManager(int maxHistory)
    : m_maxHistory(maxHistory)
{
}

UndoManager::~UndoManager()
{
    clear();
}

void UndoManager::store(const QList<StrokeItem*> &strokes, const QList<RectItem*> &rects)
{
    UndoSnapshot snap;
    for (auto *s : strokes)
        snap.strokes.append(s->copy());
    for (auto *r : rects)
        snap.rects.append(r->copy());
    m_history.append(snap);

    while (m_history.size() > m_maxHistory) {
        auto &old = m_history.first();
        for (auto *s : old.strokes) delete s;
        for (auto *r : old.rects) delete r;
        m_history.removeFirst();
    }
}

bool UndoManager::canUndo() const
{
    return m_history.size() >= 2;
}

UndoSnapshot UndoManager::undo()
{
    if (!canUndo())
        return {};

    m_history.removeLast();
    const UndoSnapshot &prev = m_history.last();
    UndoSnapshot result;
    for (auto *s : prev.strokes)
        result.strokes.append(s->copy());
    for (auto *r : prev.rects)
        result.rects.append(r->copy());
    return result;
}

void UndoManager::clear()
{
    m_history.clear();
}

void UndoManager::trimTo(int maxHistory)
{
    m_maxHistory = maxHistory;
    while (m_history.size() > m_maxHistory) {
        auto &old = m_history.first();
        for (auto *s : old.strokes) delete s;
        for (auto *r : old.rects) delete r;
        m_history.removeFirst();
    }
}
