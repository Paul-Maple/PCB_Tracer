#ifndef TRACE_H
#define TRACE_H

#include <QList>
#include <QPoint>
#include <QColor>

// Типы ячеек сетки
enum CellType {
    CELL_EMPTY,
    CELL_OBSTACLE,
    CELL_PAD,
    CELL_TRACE,
    CELL_VIA
};

// Структура ячейки сетки
struct GridCell {
    CellType type;
    int layer;
    int padId; // если это площадка
    int traceId; // если это трасса
    QColor color;
};

// Простая структура для хранения точек сетки
struct GridPoint {
    int x;
    int y;

    GridPoint() : x(0), y(0) {}
    GridPoint(int x_, int y_) : x(x_), y(y_) {}

    bool operator==(const GridPoint& other) const {
        return x == other.x && y == other.y;
    }
};

class PathFinder
{
public:
    PathFinder();
    
    // Основная функция поиска пути
    QList<GridPoint> findPath(const GridPoint& start, const GridPoint& end, int layer,
                            GridCell*** grid, int boardWidth, int boardHeight);
    
    // Проверка возможности размещения трассы
    bool canPlaceTrace(int x, int y, int layer, GridCell*** grid, int boardWidth, int boardHeight);
    
    // Волновой алгоритм (алгоритм Ли)
    QList<GridPoint> waveAlgorithm(const GridPoint& start, const GridPoint& end, int layer,
                                 GridCell*** grid, int boardWidth, int boardHeight);
    
    // Восстановление пути из матрицы расстояний
    QList<GridPoint> reconstructPath(const GridPoint& start, const GridPoint& end,
                                   int** dist, int boardWidth, int boardHeight);
    
private:
    // Направления для поиска (4-связность)
    static const int dx[4];
    static const int dy[4];
};

#endif // TRACE_H