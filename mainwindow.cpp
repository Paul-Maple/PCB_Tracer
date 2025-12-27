#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "trace.h"
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QBrush>
#include <QPen>
#include <cmath>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSizePolicy>
#include <algorithm>
#include <QDebug>

void CustomGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF scenePos = event->scenePos();
    int x = static_cast<int>(scenePos.x()) / 30;
    int y = static_cast<int>(scenePos.y()) / 30;
    emit cellClicked(x, y);
    QGraphicsScene::mousePressEvent(event);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scene(nullptr),
    layerButtonGroup(new QButtonGroup(this)),
    grid(nullptr),
    cells(nullptr)
{
    ui->setupUi(this);

    // Устанавливаем начальное количество слоев на 2
    layerCount = 2;
    ui->layerSpinBox->setValue(layerCount);

    // Создаем кастомную сцену
    scene = new CustomGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    // Создаем радио-кнопки для слоев
    createLayerRadioButtons();

    // Подключаем сигнал от кастомной сцены
    connect(scene, &CustomGraphicsScene::cellClicked,
            this, &MainWindow::onCellClicked);

    // Подключение сигналов кнопок
    connect(ui->obstacleBtn, &QPushButton::clicked, this, &MainWindow::setModeObstacle);
    connect(ui->padBtn, &QPushButton::clicked, this, &MainWindow::setModePad);
    connect(ui->connectionBtn, &QPushButton::clicked, this, &MainWindow::setModeConnection);
    connect(ui->routeBtn, &QPushButton::clicked, this, &MainWindow::onRoute);
    connect(ui->clearBtn, &QPushButton::clicked, this, &MainWindow::onClearTraces);
    connect(ui->removeObstacleBtn, &QPushButton::clicked, this, &MainWindow::onRemoveObstacle);
    connect(ui->removePadBtn, &QPushButton::clicked, this, &MainWindow::onRemovePad);
    connect(ui->boardSizeBtn, &QPushButton::clicked, this, &MainWindow::onBoardSizeChanged);
    connect(ui->layerSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onLayerCountChanged);

    // Инициализация сетки
    initGrid();
    drawGrid();
}

MainWindow::~MainWindow()
{
    // Удаляем радио-кнопки
    QList<QAbstractButton*> buttons = layerButtonGroup->buttons();
    for (QAbstractButton* btn : buttons) {
        layerButtonGroup->removeButton(btn);
    }
    delete layerButtonGroup;

    clearGrid();
    delete scene;
    delete ui;
}

void MainWindow::createLayerRadioButtons()
{
    // Создаем группу для радио-кнопок слоев
    // Проверяем, нет ли уже созданной группы
    static QGroupBox *layerGroupBox = nullptr;

    if (layerGroupBox) {
        // Если группа уже существует, удаляем старую
        ui->verticalLayout->removeWidget(layerGroupBox);
        delete layerGroupBox;
        layerGroupBox = nullptr;
    }

    layerGroupBox = new QGroupBox("Выбор слоя", this);
    QVBoxLayout *layerLayout = new QVBoxLayout(layerGroupBox);

    // Создаем радио-кнопки для каждого слоя
    for (int i = 0; i < 8; i++) { // Всегда создаем 8 кнопок, но некоторые будут отключены
        QRadioButton *radioBtn = new QRadioButton(QString("Слой %1").arg(i + 1), layerGroupBox);
        radioBtn->setStyleSheet(QString("QRadioButton { color: %1; }").arg(layerColors[i].name()));

        // Отключаем кнопки для слоев, которые больше текущего количества
        radioBtn->setEnabled(i < layerCount);

        layerButtonGroup->addButton(radioBtn, i);
        layerLayout->addWidget(radioBtn);
    }

    // Выбираем первый слой по умолчанию
    if (layerButtonGroup->button(0)) {
        layerButtonGroup->button(0)->setChecked(true);
        currentLayer = 0;
    }

    // Подключаем сигнал изменения выбора
    connect(layerButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::onLayerRadioButtonClicked);

    // Добавляем группу в правую панель на позицию 2 (после "Настройки слоев")
    ui->verticalLayout->insertWidget(2, layerGroupBox);
}

void MainWindow::onLayerRadioButtonClicked()
{
    int newLayer = layerButtonGroup->checkedId();
    if (newLayer >= 0 && newLayer < layerCount && newLayer != currentLayer) {
        currentLayer = newLayer;

        // Обновляем существующие линии трасс
        updateTraceLinesForCurrentLayer();

        drawGrid(); // Обновляем отображение ячеек
        ui->statusBar->showMessage(QString("Текущий слой: %1").arg(currentLayer + 1));
    }
}

void MainWindow::updateTraceLinesForCurrentLayer()
{
    // Обновляем все линии трасс в списке
    for (TraceLineInfo& info : traceLinesInfo) {
        if (info.line) {
            int lineLayer = info.layer;
            QColor newColor = getLayerColor(lineLayer, lineLayer == currentLayer);
            int lineWidth = (lineLayer == currentLayer) ? 4 : 2;
            Qt::PenStyle lineStyle = (lineLayer == currentLayer) ? Qt::SolidLine : Qt::DashLine;
            qreal opacity = (lineLayer == currentLayer) ? 1.0 : 0.6;

            info.line->setPen(QPen(newColor, lineWidth, lineStyle, Qt::RoundCap));
            info.line->setOpacity(opacity);
        }
    }
}

void MainWindow::initGrid()
{
    // Выделение памяти для 3D сетки
    grid = new GridCell**[static_cast<size_t>(layerCount)];
    for (int l = 0; l < layerCount; l++) {
        grid[l] = new GridCell*[static_cast<size_t>(boardHeight)];
        for (int y = 0; y < boardHeight; y++) {
            grid[l][y] = new GridCell[static_cast<size_t>(boardWidth)];
            for (int x = 0; x < boardWidth; x++) {
                grid[l][y][x] = {CELL_EMPTY, l, -1, -1, Qt::white};
            }
        }
    }

    // Инициализация визуальных ячеек
    cells = new QGraphicsRectItem**[static_cast<size_t>(boardHeight)];
    for (int y = 0; y < boardHeight; y++) {
        cells[y] = new QGraphicsRectItem*[static_cast<size_t>(boardWidth)];
        for (int x = 0; x < boardWidth; x++) {
            QGraphicsRectItem* rect = scene->addRect(
                x * gridSize, y * gridSize, gridSize, gridSize,
                QPen(Qt::black), QBrush(Qt::white)
            );
            rect->setData(0, x);
            rect->setData(1, y);

            // Убираем выделение
            rect->setFlag(QGraphicsItem::ItemIsFocusable, false);
            rect->setFlag(QGraphicsItem::ItemIsSelectable, false);

            cells[y][x] = rect;
        }
    }
}

void MainWindow::clearGrid()
{
    // Очистка данных
    pads.clear();

    // Удаляем визуальные линии связей
    for (Connection& conn : connections) {
        if (conn.visualLine) {
            scene->removeItem(conn.visualLine);
            delete conn.visualLine;
            conn.visualLine = nullptr;
        }
    }
    connections.clear();

    // Удаляем линии трасс
    for (TraceLineInfo& info : traceLinesInfo) {
        if (info.line) {
            scene->removeItem(info.line);
            delete info.line;
        }
    }
    traceLinesInfo.clear();

    nextPadId = 1;
    selectedPadId = -1;

    if (!grid) return;

    // Очистка памяти 3D сетки
    for (int l = 0; l < layerCount; l++) {
        if (grid[l]) {
            for (int y = 0; y < boardHeight; y++) {
                delete[] grid[l][y];
            }
            delete[] grid[l];
        }
    }
    delete[] grid;
    grid = nullptr;

    // Очистка визуальных ячеек
    if (cells) {
        for (int y = 0; y < boardHeight; y++) {
            delete[] cells[y];
        }
        delete[] cells;
        cells = nullptr;
    }

    // Очистка сцены
    if (scene) {
        scene->clear();
    }
}

void MainWindow::drawGrid()
{
    if (!grid || !cells) return;

    for (int y = 0; y < boardHeight; y++) {
        for (int x = 0; x < boardWidth; x++) {
            updateCellDisplay(x, y);
        }
    }
}

void MainWindow::drawTraceLine(const GridPoint& from, const GridPoint& to, int layer)
{
    if (layer < 0 || layer >= layerCount) return;

    QColor traceColor = getLayerColor(layer, layer == currentLayer);

    // Вычисляем координаты центров ячеек
    qreal x1 = from.x * gridSize + gridSize / 2.0;
    qreal y1 = from.y * gridSize + gridSize / 2.0;
    qreal x2 = to.x * gridSize + gridSize / 2.0;
    qreal y2 = to.y * gridSize + gridSize / 2.0;

    // Определяем толщину и стиль линии в зависимости от того, активен ли слой
    int lineWidth = (layer == currentLayer) ? 4 : 2;
    Qt::PenStyle lineStyle = (layer == currentLayer) ? Qt::SolidLine : Qt::DashLine;
    qreal opacity = (layer == currentLayer) ? 1.0 : 0.6; // Более яркие линии на активном слое

    // Рисуем линию между центрами ячеек
    QGraphicsLineItem* line = scene->addLine(
        x1, y1, x2, y2,
        QPen(traceColor, lineWidth, lineStyle, Qt::RoundCap)
    );

    // Сохраняем информацию о линии
    TraceLineInfo info;
    info.line = line;
    info.layer = layer;
    traceLinesInfo.append(info);

    // Устанавливаем прозрачность для неактивных слоев
    if (layer != currentLayer) {
        line->setOpacity(opacity);
    }
}

void MainWindow::drawConnectionLine(int padId1, int padId2)
{
    Pad* pad1 = getPadById(padId1);
    Pad* pad2 = getPadById(padId2);

    if (!pad1 || !pad2) return;

    // Вычисляем координаты центров ячеек
    qreal x1 = pad1->x * gridSize + gridSize / 2.0;
    qreal y1 = pad1->y * gridSize + gridSize / 2.0;
    qreal x2 = pad2->x * gridSize + gridSize / 2.0;
    qreal y2 = pad2->y * gridSize + gridSize / 2.0;

    // Рисуем тонкую черную линию между площадками
    QGraphicsLineItem* line = scene->addLine(
        x1, y1, x2, y2,
        QPen(Qt::black, 1, Qt::DashLine)
    );

    // Сохраняем ссылку на линию в connection
    for (Connection& conn : connections) {
        if ((conn.fromPadId == padId1 && conn.toPadId == padId2) ||
            (conn.fromPadId == padId2 && conn.toPadId == padId1)) {
            if (conn.visualLine) {
                scene->removeItem(conn.visualLine);
                delete conn.visualLine;
            }
            conn.visualLine = line;
            break;
        }
    }
}

void MainWindow::updateConnectionLines()
{
    // Удаляем старые линии
    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        if (QGraphicsLineItem* lineItem = dynamic_cast<QGraphicsLineItem*>(item)) {
            // Проверяем, является ли это линией связи (а не трассой)
            bool isTraceLine = false;
            QPen pen = lineItem->pen();
            if (pen.width() >= 2 || pen.color() != Qt::black || pen.style() != Qt::DashLine) {
                isTraceLine = true;
            }

            if (!isTraceLine) {
                scene->removeItem(lineItem);
                delete lineItem;
            }
        }
    }

    // Обнуляем указатели в connections
    for (Connection& conn : connections) {
        conn.visualLine = nullptr;
    }

    // Рисуем новые линии связей
    for (const Connection& conn : connections) {
        drawConnectionLine(conn.fromPadId, conn.toPadId);
    }
}

void MainWindow::updateCellDisplay(int x, int y, int layer)
{
    if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) return;
    if (layer == -1) layer = currentLayer;
    if (layer < 0 || layer >= layerCount) return;

    QGraphicsRectItem* rect = cells[y][x];
    GridCell& cell = grid[layer][y][x];

    QColor color = cell.color;
    QPen pen(Qt::black, 1);

    // Определяем, активен ли этот слой
    bool isActiveLayer = (layer == currentLayer);

    // Для трасс - ячейки должны оставаться пустыми (белыми)
    if (cell.type == CELL_TRACE) {
        color = Qt::white; // Ячейка остается белой
        pen = QPen(Qt::gray, 1); // Тонкая серая граница

        // НЕ подсвечиваем ячейки трасс
        // Вместо этого трассы будут отображаться только линиями
    }
    // Для виа всегда черный (ячейка под виа)
    else if (cell.type == CELL_VIA) {
        color = Qt::black;
        pen = QPen(Qt::black, 2);
    }
    // Для препятствий серый
    else if (cell.type == CELL_OBSTACLE) {
        color = Qt::darkGray;
        pen = QPen(Qt::darkGray, 2);
    }
    // Для площадок оставляем оригинальный цвет
    else if (cell.type == CELL_PAD) {
        pen = QPen(color.darker(), 2);
    }
    // Для пустых ячеек
    else {
        color = Qt::white;
        pen = QPen(Qt::gray, 1);
    }

    rect->setBrush(QBrush(color));
    rect->setPen(pen);

    // Сброс прозрачности
    rect->setOpacity(1.0);
}

void MainWindow::onCellClicked(int x, int y)
{
    // Проверяем границы
    if (x < 0 || x >= boardWidth || y < 0 || y >= boardHeight) {
        ui->statusBar->showMessage("Ошибка: клик вне границ платы");
        return;
    }

    switch (currentMode) {
    case MODE_OBSTACLE:
        // Проверяем, нет ли площадки в этой точке
        if (hasPadAt(x, y)) {
            QMessageBox::warning(this, "Ошибка",
                "Нельзя установить препятствие на контактной площадке");
            return;
        }

        grid[currentLayer][y][x].type = CELL_OBSTACLE;
        grid[currentLayer][y][x].color = Qt::darkGray;
        updateCellDisplay(x, y);
        ui->statusBar->showMessage(QString("Установлено препятствие в (%1, %2)").arg(x).arg(y));
        break;

    case MODE_PAD: {
        // Проверяем, нет ли уже площадки в этой точке
        if (hasPadAt(x, y)) {
            QMessageBox::warning(this, "Ошибка",
                "В этой точке уже есть контактная площадка");
            return;
        }

        // Проверяем, нет ли препятствия на текущем слое
        if (grid[currentLayer][y][x].type == CELL_OBSTACLE) {
            QMessageBox::warning(this, "Ошибка",
                "Нельзя установить площадку на препятствии");
            return;
        }

        // УБИРАЕМ ДИАЛОГОВОЕ ОКНО и используем автоматическое имя
        QString name = QString("PAD%1").arg(nextPadId);

        Pad pad;
        pad.id = nextPadId;
        pad.x = x;
        pad.y = y;
        pad.name = name;

        // Генерируем случайный цвет для площадки
        static int hue = 0;
        pad.color = QColor::fromHsv(hue, 180, 220);
        hue = (hue + 40) % 360;

        // Добавляем площадку в список
        pads.append(pad);

        // Установка на всех слоях
        for (int l = 0; l < layerCount; l++) {
            grid[l][y][x].type = CELL_PAD;
            grid[l][y][x].padId = pad.id;
            grid[l][y][x].color = pad.color;
            grid[l][y][x].traceId = -1;
        }

        nextPadId++; // Увеличиваем ID для следующей площадки

        updateCellDisplay(x, y);
        ui->statusBar->showMessage(
            QString("Добавлена площадка %1 в (%2, %3)")
            .arg(name).arg(x).arg(y));
        break;
    }

    case MODE_CONNECTION: {
        int padId = getPadAt(x, y);
        if (padId == -1) {
            QMessageBox::warning(this, "Ошибка",
                "Выберите контактную площадку");
            return;
        }

        if (selectedPadId == -1) {
            selectedPadId = padId;
            Pad* pad = getPadById(padId);
            ui->statusBar->showMessage(
                QString("Выбрана площадка %1. Выберите вторую площадку для соединения")
                .arg(pad ? pad->name : QString::number(padId)));
        } else if (selectedPadId != padId) {
            // Проверяем, нет ли уже такой связи
            bool connectionExists = false;
            for (const Connection& conn : connections) {
                if ((conn.fromPadId == selectedPadId && conn.toPadId == padId) ||
                    (conn.fromPadId == padId && conn.toPadId == selectedPadId)) {
                    connectionExists = true;
                    break;
                }
            }

            if (connectionExists) {
                QMessageBox::information(this, "Информация",
                    "Связь между этими площадками уже существует");
                selectedPadId = -1;
                return;
            }

            // Добавление связи
            Connection conn;
            conn.fromPadId = selectedPadId;
            conn.toPadId = padId;
            conn.routed = false;
            conn.layer = -1;
            conn.visualLine = nullptr;

            connections.append(conn);

            // Рисуем визуальную линию связи
            drawConnectionLine(selectedPadId, padId);

            // Обновление связей площадок
            Pad* pad1 = getPadById(selectedPadId);
            Pad* pad2 = getPadById(padId);

            if (pad1 && pad2) {
                if (!pad1->connections.contains(padId)) {
                    pad1->connections.append(padId);
                }
                if (!pad2->connections.contains(selectedPadId)) {
                    pad2->connections.append(selectedPadId);
                }

                ui->statusBar->showMessage(
                    QString("Создана связь между площадками %1 и %2")
                    .arg(pad1->name).arg(pad2->name));
            }

            selectedPadId = -1;
        }
        break;
    }

    default:
        // В режиме без инструмента показываем информацию о ячейке
        QString info = QString("Ячейка (%1, %2), Слой %3: ").arg(x).arg(y).arg(currentLayer + 1);

        CellType type = grid[currentLayer][y][x].type;
        switch (type) {
        case CELL_EMPTY: info += "Пусто"; break;
        case CELL_OBSTACLE: info += "Препятствие"; break;
        case CELL_PAD: {
            int padId = grid[currentLayer][y][x].padId;
            Pad* pad = getPadById(padId);
            info += QString("Площадка %1").arg(pad ? pad->name : "?");
            break;
        }
        case CELL_TRACE: info += "Трасса"; break;
        case CELL_VIA: info += "Переход"; break;
        }

        ui->statusBar->showMessage(info);
        break;
    }
}

void MainWindow::onRoute()
{
    if (connections.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет связей для трассировки");
        return;
    }

    // Очищаем старые трассы
    onClearTraces();

    // Выполняем многослойную трассировку
    performMultilayerRouting();

    // Удаляем линии связей после трассировки
    removeConnectionLines();

    // Обновляем отображение
    drawGrid();
    ui->statusBar->showMessage("Трассировка завершена");
}

// Исправьте вызов findPath в performMultilayerRouting:
void MainWindow::performMultilayerRouting()
{
    int routedCount = 0;

    // Сортируем соединения по сложности
    QList<Connection> sortedConnections = connections;
    std::sort(sortedConnections.begin(), sortedConnections.end(),
              [this](const Connection& a, const Connection& b) {
                  Pad* padA1 = getPadById(a.fromPadId);
                  Pad* padA2 = getPadById(a.toPadId);
                  Pad* padB1 = getPadById(b.fromPadId);
                  Pad* padB2 = getPadById(b.toPadId);

                  if (!padA1 || !padA2 || !padB1 || !padB2) return false;

                  int distA = abs(padA1->x - padA2->x) + abs(padA1->y - padA2->y);
                  int distB = abs(padB1->x - padB2->x) + abs(padB1->y - padB2->y);

                  return distA < distB;
              });

    qDebug() << "Начинаем трассировку" << sortedConnections.size() << "соединений";

    // Трассируем каждое соединение
    for (int connIndex = 0; connIndex < sortedConnections.size(); connIndex++) {
        Connection& conn = sortedConnections[connIndex];
        Pad* fromPad = getPadById(conn.fromPadId);
        Pad* toPad = getPadById(conn.toPadId);

        if (!fromPad || !toPad) {
            qDebug() << "Пропускаем соединение: не найдены площадки";
            continue;
        }

        qDebug() << "Трассируем соединение" << connIndex << "от" << fromPad->name
                 << "(ID:" << fromPad->id << ") до" << toPad->name << "(ID:" << toPad->id << ")";

        // Инициализируем точки начала и конца
        GridPoint start(fromPad->x, fromPad->y, 0);
        GridPoint end(toPad->x, toPad->y, 0);

        // ВАЖНО: передаем ОБА ID площадок
        QList<GridPoint> path = findPath(start, end, fromPad->id, toPad->id);

        qDebug() << "Найден путь длиной:" << path.size();

        if (!path.isEmpty()) {
            // Прокладываем трассу по найденному пути
            for (int i = 0; i < path.size(); i++) {
                GridPoint point = path[i];

                // Проверяем границы
                if (point.x < 0 || point.x >= boardWidth ||
                    point.y < 0 || point.y >= boardHeight ||
                    point.layer < 0 || point.layer >= layerCount) {
                    qDebug() << "Точка пути вне границ:" << point.x << point.y << point.layer;
                    continue;
                }

                GridCell& cell = grid[point.layer][point.y][point.x];

                // Если это не площадка и не via, отмечаем как трассу
                if (cell.type != CELL_PAD && cell.type != CELL_VIA) {
                    cell.type = CELL_TRACE;
                    // Сохраняем идентификатор соединения (ID первой площадки)
                    cell.traceId = fromPad->id;
                }

                // Обработка переходов между слоями
                if (i > 0 && path[i-1].layer != point.layer) {
                    qDebug() << "Переход между слоями" << path[i-1].layer << "->" << point.layer;

                    // Делаем via на всех промежуточных слоях
                    int minLayer = qMin(path[i-1].layer, point.layer);
                    int maxLayer = qMax(path[i-1].layer, point.layer);

                    for (int l = minLayer; l <= maxLayer; l++) {
                        if (l >= 0 && l < layerCount) {
                            if (grid[l][point.y][point.x].type != CELL_PAD) {
                                grid[l][point.y][point.x].type = CELL_VIA;
                                grid[l][point.y][point.x].color = Qt::black;
                            }
                        }
                    }
                }

                updateCellDisplay(point.x, point.y, point.layer);
            }

            // Рисуем линии трасс
            for (int i = 0; i < path.size() - 1; i++) {
                drawTraceLine(path[i], path[i + 1], path[i].layer);
            }

            // Обновляем информацию о соединении
            conn.routed = true;
            conn.layer = path.last().layer;
            routedCount++;

            qDebug() << "Соединение" << fromPad->name << "-" << toPad->name << "проложено";
        } else {
            qDebug() << "Не удалось найти путь для соединения" << fromPad->name << "-" << toPad->name;
            ui->statusBar->showMessage(QString("Не удалось найти путь для соединения %1-%2")
                .arg(fromPad->name).arg(toPad->name));
        }
    }

    ui->statusBar->showMessage(QString("Проложено трасс: %1 из %2").arg(routedCount).arg(connections.size()));
    qDebug() << "Трассировка завершена:" << routedCount << "из" << connections.size() << "соединений";
}

void MainWindow::onClearTraces()
{
    // Очищаем линии трасс
    for (TraceLineInfo& info : traceLinesInfo) {
        if (info.line) {
            scene->removeItem(info.line);
            delete info.line;
        }
    }
    traceLinesInfo.clear();

    // Очищаем ячейки трасс и виа
    for (int l = 0; l < layerCount; l++) {
        for (int y = 0; y < boardHeight; y++) {
            for (int x = 0; x < boardWidth; x++) {
                if (grid[l][y][x].type == CELL_TRACE || grid[l][y][x].type == CELL_VIA) {
                    grid[l][y][x].type = CELL_EMPTY;
                    grid[l][y][x].traceId = -1;
                    grid[l][y][x].color = Qt::white;
                    updateCellDisplay(x, y, l);
                }
            }
        }
    }

    // Сбрасываем статус связей
    for (Connection& conn : connections) {
        conn.routed = false;
        conn.layer = -1;
    }

    // Восстанавливаем линии связей (если они были удалены)
    updateConnectionLines();

    drawGrid();
    ui->statusBar->showMessage("Все трассы удалены");
}

void MainWindow::onRemovePad()
{
    currentMode = MODE_NONE;

    if (pads.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет площадок для удаления");
        return;
    }

    QStringList padNames;
    for (const Pad& pad : pads) {
        padNames << QString("%1 (%2,%3)").arg(pad.name).arg(pad.x).arg(pad.y);
    }

    bool ok;
    QString selected = QInputDialog::getItem(
        this,
        "Удаление площадки",
        "Выберите площадку для удаления:",
        padNames,
        0,
        false,
        &ok
    );

    if (ok && !selected.isEmpty()) {
        int index = padNames.indexOf(selected);
        if (index != -1) {
            int padId = pads[index].id;

            // Удаление связей
            QList<Connection> newConnections;
            for (const Connection& conn : connections) {
                if (conn.fromPadId != padId && conn.toPadId != padId) {
                    newConnections.append(conn);
                } else {
                    // Удаляем визуальную линию
                    if (conn.visualLine) {
                        scene->removeItem(conn.visualLine);
                        delete conn.visualLine;
                    }
                }
            }
            connections = newConnections;

            // Удаление из сетки
            for (int l = 0; l < layerCount; l++) {
                for (int y = 0; y < boardHeight; y++) {
                    for (int x = 0; x < boardWidth; x++) {
                        if (grid[l][y][x].padId == padId) {
                            grid[l][y][x].type = CELL_EMPTY;
                            grid[l][y][x].padId = -1;
                            grid[l][y][x].color = Qt::white;
                        }
                    }
                }
            }

            pads.removeAt(index);
            drawGrid();
            updateConnectionLines(); // Обновляем линии связей
            ui->statusBar->showMessage("Площадка удалена");
        }
    }
}

void MainWindow::onLayerCountChanged(int count)
{
    clearGrid();
    layerCount = qMin(count, 8);
    layerCount = qMax(layerCount, 1);

    ui->layerSpinBox->setValue(layerCount);

    // Обновляем радио-кнопки
    QList<QAbstractButton*> buttons = layerButtonGroup->buttons();
    for (QAbstractButton* btn : buttons) {
        layerButtonGroup->removeButton(btn);
        delete btn;
    }

    createLayerRadioButtons();

    initGrid();
    drawGrid();

    ui->statusBar->showMessage(QString("Установлено %1 слоев").arg(layerCount));
}

void MainWindow::onBoardSizeChanged()
{
    bool okWidth, okHeight;
    int newWidth = QInputDialog::getInt(
        this,
        "Размер платы",
        "Ширина (количество клеток):",
        boardWidth,
        5,
        100,
        1,
        &okWidth
    );

    int newHeight = QInputDialog::getInt(
        this,
        "Размер платы",
        "Высота (количество клеток):",
        boardHeight,
        5,
        100,
        1,
        &okHeight
    );

    if (okWidth && okHeight && (newWidth != boardWidth || newHeight != boardHeight)) {
        boardWidth = newWidth;
        boardHeight = newHeight;

        clearGrid();
        initGrid();
        drawGrid();

        ui->statusBar->showMessage(QString("Размер платы изменен: %1x%2").arg(boardWidth).arg(boardHeight));
    }
}

void MainWindow::onRemoveObstacle()
{
    currentMode = MODE_NONE;

    for (int y = 0; y < boardHeight; y++) {
        for (int x = 0; x < boardWidth; x++) {
            for (int l = 0; l < layerCount; l++) {
                if (grid[l][y][x].type == CELL_OBSTACLE) {
                    grid[l][y][x].type = CELL_EMPTY;
                    grid[l][y][x].color = Qt::white;
                }
            }
        }
    }

    drawGrid();
    ui->statusBar->showMessage("Все препятствия удалены");
}

void MainWindow::setModeObstacle()
{
    currentMode = MODE_OBSTACLE;
    ui->statusBar->showMessage("Режим: установка препятствий. Кликните на ячейку.");
}

void MainWindow::setModePad()
{
    currentMode = MODE_PAD;
    ui->statusBar->showMessage("Режим: установка контактных площадок. Кликните на ячейку.");
}

void MainWindow::setModeConnection()
{
    currentMode = MODE_CONNECTION;
    selectedPadId = -1;
    ui->statusBar->showMessage("Режим: создание связей. Выберите первую площадку.");
}

void MainWindow::onSetObstacle()
{
    setModeObstacle();
}

void MainWindow::onSetPad()
{
    setModePad();
}

void MainWindow::onSetConnection()
{
    setModeConnection();
}

// Исправьте объявления функций в конце mainwindow.cpp:
QList<GridPoint> MainWindow::findPath(const GridPoint& start, const GridPoint& end, int fromPadId, int toPadId)
{
    return pathFinder.findPath(start, end, grid, boardWidth, boardHeight, layerCount, fromPadId, toPadId);
}

bool MainWindow::canPlaceTrace(int x, int y, int layer, int fromPadId, int toPadId)
{
    return pathFinder.canPlaceTrace(x, y, layer, grid, boardWidth, boardHeight, fromPadId, toPadId);
}

void MainWindow::placeVia(int x, int y)
{
    for (int l = 0; l < layerCount; l++) {
        // Не ставим переход на препятствия или другие трассы
        if (grid[l][y][x].type == CELL_OBSTACLE || grid[l][y][x].type == CELL_TRACE) {
            continue;
        }

        // Сохраняем оригинальный тип если это площадка
        if (grid[l][y][x].type == CELL_PAD) {
            continue;
        }

        grid[l][y][x].type = CELL_VIA;
        grid[l][y][x].color = Qt::black;
        updateCellDisplay(x, y, l);
    }
}

Pad* MainWindow::getPadById(int id)
{
    for (int i = 0; i < pads.size(); i++) {
        if (pads[i].id == id) {
            return &pads[i];
        }
    }
    return nullptr;
}

int MainWindow::getPadAt(int x, int y)
{
    // Просто ищем в списке площадок
    for (const Pad& pad : pads) {
        if (pad.x == x && pad.y == y) {
            return pad.id;
        }
    }
    return -1;
}

bool MainWindow::hasPadAt(int x, int y)
{
    // Простая проверка по списку площадок
    for (const Pad& pad : pads) {
        if (pad.x == x && pad.y == y) {
            return true;
        }
    }

    // Дополнительная проверка по сетке (на всякий случай)
    for (int l = 0; l < layerCount; l++) {
        if (grid[l][y][x].type == CELL_PAD) {
            return true;
        }
    }

    return false;
}

void MainWindow::updatePadConnections()
{
    // Очищаем существующие связи в площадках
    for (Pad& pad : pads) {
        pad.connections.clear();
    }

    // Обновляем связи на основе connections
    for (const Connection& conn : connections) {
        Pad* pad1 = getPadById(conn.fromPadId);
        Pad* pad2 = getPadById(conn.toPadId);

        if (pad1 && pad2) {
            if (!pad1->connections.contains(conn.toPadId)) {
                pad1->connections.append(conn.toPadId);
            }
            if (!pad2->connections.contains(conn.fromPadId)) {
                pad2->connections.append(conn.fromPadId);
            }
        }
    }
}

QColor MainWindow::getLayerColor(int layer, bool isActiveLayer)
{
    if (layer < 0 || layer >= layerColors.size()) {
        return Qt::white;
    }

    QColor baseColor = layerColors[layer];

    if (isActiveLayer) {
        // Яркий цвет для активного слоя
        return baseColor.lighter(150);
    } else {
        // Тусклый цвет для неактивных слоев
        QColor dimmed = baseColor.darker(200);
        dimmed.setAlpha(150); // Полупрозрачный
        return dimmed;
    }
}

void MainWindow::removeConnectionLines()
{
    // Удаляем все визуальные линии связей
    for (Connection& conn : connections) {
        if (conn.visualLine) {
            scene->removeItem(conn.visualLine);
            delete conn.visualLine;
            conn.visualLine = nullptr;
        }
    }
}
