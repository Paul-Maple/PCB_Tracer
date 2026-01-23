#ifndef ROUTINGTASK_H
#define ROUTINGTASK_H

#include "trace.h"
#include <QRunnable>
#include <QMutex>

class RoutingTask : public QRunnable
{
public:
    RoutingTask(GridCell*** grid, int boardWidth, int boardHeight, int totalLayers,
                const GridPoint& start, const GridPoint& end,
                int fromPadId, int toPadId, QMutex* gridMutex = nullptr);

    QList<GridPoint> getPath() const { return path; }
    bool isSuccess() const { return !path.isEmpty(); }

    void run() override;

private:
    GridCell*** grid;
    int boardWidth;
    int boardHeight;
    int totalLayers;
    GridPoint start;
    GridPoint end;
    int fromPadId;
    int toPadId;
    QMutex* gridMutex;
    QList<GridPoint> path;
    PathFinder pathFinder;
};

#endif // ROUTINGTASK_H
