#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include "trace.h"

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

private slots:
    void onCellClicked(int x, int y);

    // Обработчики кнопок
    void onSetObstacle();
    void onSetPad();
    void onSetConnection();
    void onRoute();
    void onClearTraces();
    void onRemoveObstacle();
    void onRemovePad();
    void onLayerCountChanged(int count);
    void onBoardSizeChanged();

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
void updateTraceLinesForCurrentLayer();
    // Параметры платы
    int gridSize = 30;
    int boardWidth = 10;
    int boardHeight = 10;
    int layerCount = 2;
    int currentLayer = 0;

    // Режимы работы
    enum Mode { MODE_NONE, MODE_OBSTACLE, MODE_PAD, MODE_CONNECTION };
    Mode currentMode = MODE_NONE;

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

    // Методы для работы с трассировкой (с новой сигнатурой)
    QList<GridPoint> findPath(const GridPoint& start, const GridPoint& end, int currentPadId);
    bool canPlaceTrace(int x, int y, int layer, int currentPadId);
    void placeVia(int x, int y);

    // Новые методы для алгоритма Хейса
    void performMultilayerRouting();
    void updateCellDisplayWithLayerHighlight(int x, int y, int layer = -1);
    QColor getLayerColor(int layer, bool isActiveLayer);
    void removeConnectionLines();

    // Вспомогательные методы
    Pad* getPadById(int id);
    int getPadAt(int x, int y);
    bool hasPadAt(int x, int y);
    void updatePadConnections();

    // Создание элементов UI для слоев
    void createLayerRadioButtons();
};

#endif // MAINWINDOW_H
