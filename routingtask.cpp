#include "routingtask.h"
#include <QDebug>
#include <algorithm>

RoutingTask::RoutingTask(GridCell*** grid, int boardWidth, int boardHeight, int totalLayers,
                         const GridPoint& start, const GridPoint& end,
                         int fromPadId, int toPadId, QMutex* gridMutex)
    : grid(grid), boardWidth(boardWidth), boardHeight(boardHeight), totalLayers(totalLayers),
      start(start), end(end), fromPadId(fromPadId), toPadId(toPadId), gridMutex(gridMutex)
{
    setAutoDelete(true);
}

void RoutingTask::run()
{
    // Ищем путь с оптимизацией по слоям
    QList<GridPoint> bestPath;
    int bestScore = INT_MAX;

    // Пробуем разные комбинации начальных и конечных слоев
    for (int startLayer = 0; startLayer < totalLayers; startLayer++) {
        for (int endLayer = 0; endLayer < totalLayers; endLayer++) {
            GridPoint newStart(start.x, start.y, startLayer);
            GridPoint newEnd(end.x, end.y, endLayer);

            QList<GridPoint> currentPath;
            {
                QMutexLocker locker(gridMutex);
                currentPath = pathFinder.findPath(newStart, newEnd, grid,
                                                 boardWidth, boardHeight,
                                                 totalLayers, fromPadId, toPadId);
            }

            if (!currentPath.isEmpty()) {
                // Оцениваем качество пути
                int score = currentPath.size();

                // Учитываем количество переходов между слоями
                int layerChanges = 0;
                int currentLayer = currentPath[0].layer;
                for (int i = 1; i < currentPath.size(); i++) {
                    if (currentPath[i].layer != currentLayer) {
                        layerChanges++;
                        currentLayer = currentPath[i].layer;
                    }
                }

                // Учитываем использование разных слоев
                QSet<int> usedLayers;
                for (const GridPoint& p : currentPath) {
                    usedLayers.insert(p.layer);
                }

                // Формула оценки: длина + штраф за смены слоев + поощрение за использование разных слоев
                score += layerChanges * 20; // Штраф за переходы
                score += (usedLayers.size() - 1) * 10; // Поощрение за использование разных слоев

                if (score < bestScore) {
                    bestScore = score;
                    bestPath = currentPath;
                }
            }
        }
    }

    path = bestPath;
}
