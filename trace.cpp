#include "trace.h"
#include <QDebug>

// Направления для поиска (4-связность)
const int PathFinder::dx[4] = {1, -1, 0, 0};
const int PathFinder::dy[4] = {0, 0, 1, -1};

PathFinder::PathFinder()
{
}

QList<GridPoint> PathFinder::findPath(const GridPoint& start, const GridPoint& end, int layer,
                                     GridCell*** grid, int boardWidth, int boardHeight)
{
    // Проверяем границы
    if (start.x < 0 || start.x >= boardWidth || start.y < 0 || start.y >= boardHeight ||
        end.x < 0 || end.x >= boardWidth || end.y < 0 || end.y >= boardHeight) {
        return QList<GridPoint>();
    }

    // Проверяем возможность размещения трассы в начальной и конечной точках
    if (!canPlaceTrace(start.x, start.y, layer, grid, boardWidth, boardHeight) ||
        !canPlaceTrace(end.x, end.y, layer, grid, boardWidth, boardHeight)) {
        return QList<GridPoint>();
    }

    // Используем волновой алгоритм для поиска пути
    return waveAlgorithm(start, end, layer, grid, boardWidth, boardHeight);
}

bool PathFinder::canPlaceTrace(int x, int y, int layer, GridCell*** grid, int boardWidth, int boardHeight)
{
    // Проверяем границы
    if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) {
        return false;
    }

    // Проверяем тип ячейки
    CellType type = grid[layer][y][x].type;

    // Можно разместить трассу, если ячейка пустая
    // или это площадка (можно подключиться)
    bool canPlace = (type == CELL_EMPTY || type == CELL_PAD || type == CELL_VIA);

    return canPlace;
}

QList<GridPoint> PathFinder::waveAlgorithm(const GridPoint& start, const GridPoint& end, int layer,
                                         GridCell*** grid, int boardWidth, int boardHeight)
{
    QList<GridPoint> path;

    // Инициализируем матрицу расстояний
    int** dist = new int*[static_cast<size_t>(boardHeight)];
    for (int y = 0; y < boardHeight; y++) {
        dist[y] = new int[static_cast<size_t>(boardWidth)];
        for (int x = 0; x < boardWidth; x++) {
            dist[y][x] = -1;
        }
    }

    // Очередь для волнового алгоритма
    QList<GridPoint> queue;
    dist[start.y][start.x] = 0;
    queue.append(start);

    // Волновой алгоритм (алгоритм Ли)
    bool found = false;
    while (!queue.isEmpty() && !found) {
        GridPoint current = queue.takeFirst();

        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx >= 0 && nx < boardWidth && ny >= 0 && ny < boardHeight) {
                // Проверяем, достигли ли конечной точки или можно ли пройти через ячейку
                if ((nx == end.x && ny == end.y) || canPlaceTrace(nx, ny, layer, grid, boardWidth, boardHeight)) {
                    if (dist[ny][nx] == -1) {
                        dist[ny][nx] = dist[current.y][current.x] + 1;
                        queue.append(GridPoint(nx, ny));

                        if (nx == end.x && ny == end.y) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Восстанавливаем путь если нашли
    if (found) {
        path = reconstructPath(start, end, dist, boardWidth, boardHeight);
    }

    // Очистка памяти
    for (int y = 0; y < boardHeight; y++) {
        delete[] dist[y];
    }
    delete[] dist;

    return path;
}

QList<GridPoint> PathFinder::reconstructPath(const GridPoint& start, const GridPoint& end,
                                           int** dist, int boardWidth, int boardHeight)
{
    QList<GridPoint> path;

    GridPoint current = end;
    while (!(current.x == start.x && current.y == start.y)) {
        path.prepend(current);

        // Ищем предыдущую точку
        bool foundPrev = false;
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx >= 0 && nx < boardWidth && ny >= 0 && ny < boardHeight) {
                if (dist[ny][nx] == dist[current.y][current.x] - 1) {
                    current = GridPoint(nx, ny);
                    foundPrev = true;
                    break;
                }
            }
        }

        if (!foundPrev) {
            // Не удалось восстановить путь
            return QList<GridPoint>();
        }
    }
    path.prepend(start);

    return path;
}