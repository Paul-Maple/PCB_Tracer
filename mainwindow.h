#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef _OPENMP
#include <omp.h>
#endif

#include <QScreen>
#include <QGuiApplication>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QList>
#include <QColor>
#include <QMap>
#include <QGraphicsSceneMouseEvent>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDialog>
#include <QSpinBox>
#include <QElapsedTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include "trace.h"
#include "helpdialog.h"

namespace Ui {
class MainWindow;
}

struct Pad {
    int id;
    int x;
    int y;
    QString name;
    QColor color;
    QList<int> connections;
};

struct Connection {
    int fromPadId;
    int toPadId;
    bool routed;
    int layer;
    QGraphicsLineItem* visualLine;
};

struct TraceLineInfo {
    QGraphicsLineItem* line;
    int layer;
};

// Класс для выполнения трассировки в отдельном потоке
class CustomGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit CustomGraphicsScene(QObject *parent = nullptr) : QGraphicsScene(parent) {}

signals:
    void cellClicked(int x, int y);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    struct RoutingResult {
        int connIndex;
        QList<GridPoint> path;
        bool success;
        int fromPadId;
        int toPadId;

        RoutingResult() : connIndex(-1), success(false), fromPadId(-1), toPadId(-1) {}
    };

    struct SafeGridAccess {
        GridCell*** grid = nullptr;
        int boardWidth = 0;
        int boardHeight = 0;
        int layerCount = 0;
        QMutex* mutex = nullptr;

        // Функция для безопасного доступа к ячейке
        GridCell& cell(int x, int y, int layer) {
            Q_ASSERT(mutex);
            QMutexLocker locker(mutex);
            Q_ASSERT(x >= 0 && x < boardWidth);
            Q_ASSERT(y >= 0 && y < boardHeight);
            Q_ASSERT(layer >= 0 && layer < layerCount);
            return grid[layer][y][x];
        }

        // Проверка возможности размещения трассы (inline определение)
        bool canPlaceTrace(int x, int y, int layer, int fromPadId, int toPadId) {
            QMutexLocker locker(mutex);
            if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight || layer < 0 || layer >= layerCount) {
                return false;
            }

            const GridCell& cell = grid[layer][y][x];

            // Для площадок: разрешаем доступ только к площадкам этого соединения
            if (cell.type == CELL_PAD) {
                return (cell.padId == fromPadId || cell.padId == toPadId);
            }

            // Для препятствий
            if (cell.type == CELL_OBSTACLE) {
                return false;
            }

            // Для трасс и VIA: можно использовать, если они свободны или принадлежат нам
            if (cell.type == CELL_TRACE || cell.type == CELL_VIA) {
                return (cell.traceId == -1 || cell.traceId == fromPadId);
            }

            // Пустые ячейки всегда доступны
            return true;
        }

        // Размещение трассы (inline определение)
        void placeTrace(int x, int y, int layer, int traceId) {
            QMutexLocker locker(mutex);
            if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight || layer < 0 || layer >= layerCount) {
                return;
            }

            GridCell& cell = grid[layer][y][x];
            if (cell.type == CELL_EMPTY) {
                cell.type = CELL_TRACE;
                cell.traceId = traceId;
                cell.color = Qt::white;
            }
        }

        // Размещение виа (inline определение)
        void placeVia(int x, int y) {
            QMutexLocker locker(mutex);
            if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) {
                return;
            }

            // Помечаем как VIA на всех слоях
            for (int l = 0; l < layerCount; l++) {
                GridCell& cell = grid[l][y][x];
                if (cell.type == CELL_EMPTY) {
                    cell.type = CELL_VIA;
                    cell.color = Qt::white;
                }
            }
        }
    };

private slots:
    void onHelpClicked();
    void onCellClicked(int x, int y);
    void showFailedConnections();
    // Обработчики кнопок
    void restoreFailedConnectionLines();
    void onSetObstacle();
    void onSetPad();
    void onSetConnection();
    void onRoute();
    void onClearTraces();
    void onRemoveObstacle();
    void onRemovePad();
    void onLayerCountChanged(int count);
    void onBoardSizeChanged();
    void onAutoFill();
    void onRoutingMethodChanged();  // Новый слот

    void processRoutingResultsImproved(const QList<RoutingResult>& results,
                                                  const QList<Connection>& connectionsToRoute);
    // Режимы работы
    void setModeObstacle();
    void setModePad();
    void setModeConnection();

    // Обработчик переключения слоев
    void onLayerRadioButtonClicked();

private:
    Ui::MainWindow *ui;
    CustomGraphicsScene *scene;
    QButtonGroup *layerButtonGroup;
    PathFinder pathFinder;
    QList<TraceLineInfo> traceLinesInfo;
    QElapsedTimer routeTimer;  // Добавляем здесь
    void updateTraceLinesForCurrentLayer();

    // Измените эту строку: убрали const
    RoutingResult routeSingleConnectionAsync(int connIndex, const Connection& conn);

    bool canPlacePathInTempGrid(const QList<GridPoint>& path,
                                   GridCell*** tempGrid,
                                   int fromPadId, int toPadId);
        void placePathInTempGrid(const QList<GridPoint>& path,
                                GridCell*** tempGrid,
                                int traceId);
        void copyTempGridToMain(GridCell*** tempGrid);
    int findLayerForPad(int x, int y);
    void placeTraceSafely(const QList<GridPoint>& path, int fromPadId, int toPadId);
    // Многопоточная трассировка
    QList<RoutingResult> routeConnectionsParallel(const QList<Connection>& connectionsToRoute);
    void processRoutingResults(const QList<RoutingResult>& results);
    void routeSingleConnection(Connection& conn);  // Добавьте эту строку
    // Для многопоточной оптимизации
    GridCell*** createTempGrid();
    void freeTempGrid(GridCell*** tempGrid);
    int findOptimalLayer(int x, int y);

    // Для безопасного доступа к сетке в многопоточном режиме
    struct {
        GridCell*** grid = nullptr;
        int boardWidth = 0;
        int boardHeight = 0;
        int layerCount = 0;
        QMutex* mutex = nullptr;

        // Функция для безопасного доступа к ячейке
        GridCell& cell(int x, int y, int layer) {
            Q_ASSERT(mutex);
            QMutexLocker locker(mutex);
            Q_ASSERT(x >= 0 && x < boardWidth);
            Q_ASSERT(y >= 0 && y < boardHeight);
            Q_ASSERT(layer >= 0 && layer < layerCount);
            return grid[layer][y][x];
        }

        // Проверка возможности размещения трассы
        bool canPlaceTrace(int x, int y, int layer, int fromPadId, int toPadId);

        // Размещение трассы
        void placeTrace(int x, int y, int layer, int traceId);

        // Размещение виа
        void placeVia(int x, int y);
    } safeGrid;

    // Параметры платы
    int gridSize = 30;
    int boardWidth = 20;
    int boardHeight = 20;
    int layerCount = 2;
    int currentLayer = 0;
    QList<int> failedConnections;

    // Режимы работы
    enum Mode { MODE_NONE, MODE_OBSTACLE, MODE_PAD, MODE_CONNECTION };
    Mode currentMode = MODE_NONE;

    // Метод трассировки
    enum RoutingMethod { SINGLE_THREADED, MULTI_THREADED };
    RoutingMethod currentRoutingMethod = SINGLE_THREADED;

    // Для многопоточной трассировки
    QMutex gridMutex;
    QThreadPool threadPool;
    QList<QFutureWatcher<QList<GridPoint>>*> futureWatchers;

    // Данные
    QList<Pad> pads;
    QList<Connection> connections;
    GridCell*** grid;

    // Визуализация
    QGraphicsRectItem*** cells;
    int nextPadId = 1;
    int selectedPadId = -1;

    // Цвета для слоев
    QList<QColor> layerColors = {
        QColor(255, 0, 0),
        QColor(0, 255, 0),
        QColor(0, 0, 255),
        QColor(255, 255, 0),
        QColor(255, 0, 255),
        QColor(0, 255, 255),
        QColor(255, 165, 0),
        QColor(128, 0, 128)
    };

    // Инициализация
    void initGrid();
    void clearGrid();
    void drawGrid();
    void updateCellDisplay(int x, int y, int layer = -1);
    void drawTraceLine(const GridPoint& from, const GridPoint& to, int layer);
    void drawConnectionLine(int padId1, int padId2);
    void updateConnectionLines();

    // Методы для работы с трассировкой
    QList<GridPoint> findPath(const GridPoint& start, const GridPoint& end, int fromPadId, int toPadId);
    void placeVia(int x, int y);
    bool canPlaceTrace(int x, int y, int layer, int fromPadId, int toPadId);

    // Алгоритмы трассировки
    void performSingleThreadedRouting();
    void performMultiThreadedRouting();

    int countLayerTransitions(const QList<GridPoint>& path);
        int estimateConnectionComplexity(int x1, int y1, int x2, int y2);
        bool tryPlacePathWithRetry(const QList<GridPoint>& originalPath,
                                  int fromPadId, int toPadId,
                                  Connection& conn);
        QList<GridPoint> adjustPathAroundConflict(const QList<GridPoint>& originalPath,
                                                 int conflictIndex,
                                                 int fromPadId, int toPadId);
    void processMultiThreadedResults(const QList<RoutingResult>& results,
                                       const QList<Connection>& connectionsToRoute);
   void drawTraceLineMultiThreaded(const GridPoint& from, const GridPoint& to, int layer);
    QList<GridPoint>routeConnection(Pad* fromPad, Pad* toPad);
    void processRoutedPath(const QList<GridPoint>& path, Connection& conn, Pad* fromPad);

    void updateCellDisplayWithLayerHighlight(int x, int y, int layer = -1);
    QColor getLayerColor(int layer, bool isActiveLayer);
    void removeConnectionLines();

    // Вспомогательные методы
    Pad* getPadById(int id);
    int getPadAt(int x, int y);
    bool hasPadAt(int x, int y);
    void updatePadConnections();

    // Авто-заполнение
    void autoFillBoard();
    QList<GridPoint> generateObstacle(int size, int type);

    // Создание элементов UI для слоев
    void createLayerRadioButtons();
};

class BoardSizeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BoardSizeDialog(QWidget *parent = nullptr, int currentWidth = 10, int currentHeight = 10);
    int getWidth() const { return widthSpinBox->value(); }
    int getHeight() const { return heightSpinBox->value(); }

private:
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
};

#endif // MAINWINDOW_H
