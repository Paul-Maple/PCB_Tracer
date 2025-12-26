#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "trace.h"  // Добавляем заголовочный файл трассировки
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
#include <QSizePolicy>  // Добавляем для управления размером

// Реализация метода mousePressEvent для CustomGraphicsScene
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
        drawGrid(); // Обновляем отображение
        ui->statusBar->showMessage(QString("Текущий слой: %1").arg(currentLayer + 1));
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

    QColor traceColor = layerColors[layer];

    // Вычисляем координаты центров ячеек
    qreal x1 = from.x * gridSize + gridSize / 2.0;
    qreal y1 = from.y * gridSize + gridSize / 2.0;
    qreal x2 = to.x * gridSize + gridSize / 2.0;
    qreal y2 = to.y * gridSize + gridSize / 2.0;

    // Рисуем тонкую линию между центрами ячеек
    scene->addLine(
        x1, y1, x2, y2,
        QPen(traceColor, 3, Qt::SolidLine, Qt::RoundCap)
    );
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

    // Для визуализации: если ячейка пустая на текущем слое,
    // показываем самый верхний непустой слой
    if (cell.type == CELL_EMPTY && layer == currentLayer) {
        for (int l = layerCount - 1; l >= 0; l--) {
            if (grid[l][y][x].type != CELL_EMPTY) {
                cell = grid[l][y][x]; // Используем ячейку с верхнего слоя
                color = cell.color;
                break;
            }
        }
    }

    // Настройка пера в зависимости от типа ячейки
    switch (cell.type) {
    case CELL_OBSTACLE:
        pen = QPen(Qt::darkGray, 2);
        color = Qt::darkGray;
        break;
    case CELL_TRACE:
        // Для трасс показываем только на их слое
        if (layer == cell.layer) {
            // Ячейки трасс оставляем белыми, линии рисуем отдельно
            pen = QPen(Qt::gray, 1);
            color = Qt::white;
        } else {
            // На других слоях трассы не видны
            pen = QPen(Qt::gray, 1);
            color = Qt::white;
        }
        break;
    case CELL_VIA:
        pen = QPen(Qt::black, 3);
        color = Qt::black;
        break;
    case CELL_PAD:
        // Площадки всегда видны
        pen = QPen(color.darker(), 2);
        break;
    default:
        pen = QPen(Qt::gray, 1);
        break;
    }

    rect->setBrush(QBrush(color));
    rect->setPen(pen);
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

        // Создаем диалоговое окно для ввода имени площадки
        bool ok;
        QString name = QInputDialog::getText(
            this,
            "Контактная площадка",
            "Введите имя площадки:",
            QLineEdit::Normal,
            QString("PAD%1").arg(nextPadId),
            &ok
        );

        // Устанавливаем минимальный размер для диалога
        if (ok && !name.isEmpty()) {
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
        }
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

    // Очищаем сцену от старых линий трасс (но оставляем линии связей)
    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        if (QGraphicsLineItem* lineItem = dynamic_cast<QGraphicsLineItem*>(item)) {
            QPen pen = lineItem->pen();
            // Удаляем только линии трасс (толстые и цветные)
            if (pen.width() >= 2 && pen.color() != Qt::black) {
                scene->removeItem(lineItem);
                delete lineItem;
            }
        }
    }

    int routedCount = 0;

    // Пробуем трассировать каждую связь
    for (Connection& conn : connections) {
        Pad* fromPad = getPadById(conn.fromPadId);
        Pad* toPad = getPadById(conn.toPadId);

        if (!fromPad || !toPad) {
            continue;
        }

        GridPoint start(fromPad->x, fromPad->y);
        GridPoint end(toPad->x, toPad->y);

        // Используем PathFinder для поиска пути
        QList<GridPoint> path = findPath(start, end, currentLayer);

        if (!path.isEmpty()) {
            // Очищаем ячейки от старых трасс на текущем слое
            for (int i = 0; i < path.size(); i++) {
                GridPoint point = path[i];
                if (grid[currentLayer][point.y][point.x].type != CELL_PAD) {
                    grid[currentLayer][point.y][point.x].type = CELL_EMPTY;
                    grid[currentLayer][point.y][point.x].color = Qt::white;
                }
            }

            // Рисуем тонкие линии между точками пути
            for (int i = 0; i < path.size() - 1; i++) {
                drawTraceLine(path[i], path[i + 1], currentLayer);
            }

            // Помечаем ячейки как трассы (но не закрашиваем их)
            for (int i = 0; i < path.size(); i++) {
                GridPoint point = path[i];
                // Не перезаписываем площадки
                if (grid[currentLayer][point.y][point.x].type != CELL_PAD) {
                    grid[currentLayer][point.y][point.x].type = CELL_TRACE;
                    grid[currentLayer][point.y][point.x].traceId = conn.fromPadId;
                    grid[currentLayer][point.y][point.x].color = layerColors[currentLayer];
                }
            }

            conn.routed = true;
            conn.layer = currentLayer;
            routedCount++;

            // Обновляем отображение всех затронутых ячеек
            for (int i = 0; i < path.size(); i++) {
                GridPoint point = path[i];
                updateCellDisplay(point.x, point.y, currentLayer);
            }
        }
    }

    ui->statusBar->showMessage(QString("Проложено трасс: %1 из %2").arg(routedCount).arg(connections.size()));
}

void MainWindow::onClearTraces()
{
    // Очищаем линии трасс (но оставляем линии связей)
    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        if (QGraphicsLineItem* lineItem = dynamic_cast<QGraphicsLineItem*>(item)) {
            QPen pen = lineItem->pen();
            // Удаляем только линии трасс (толстые и цветные)
            if (pen.width() >= 2 && pen.color() != Qt::black) {
                scene->removeItem(lineItem);
                delete lineItem;
            }
        }
    }

    for (int l = 0; l < layerCount; l++) {
        for (int y = 0; y < boardHeight; y++) {
            for (int x = 0; x < boardWidth; x++) {
                if (grid[l][y][x].type == CELL_TRACE) {
                    grid[l][y][x].type = CELL_EMPTY;
                    grid[l][y][x].traceId = -1;
                    grid[l][y][x].color = Qt::white;
                }
            }
        }
    }

    for (Connection& conn : connections) {
        conn.routed = false;
        conn.layer = -1;
    }

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

// Добавляем новую функцию для изменения размера платы
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

// Обёртки для методов трассировки, которые теперь используют PathFinder
QList<GridPoint> MainWindow::findPath(const GridPoint& start, const GridPoint& end, int layer)
{
    return pathFinder.findPath(start, end, layer, grid, boardWidth, boardHeight);
}

bool MainWindow::canPlaceTrace(int x, int y, int layer)
{
    return pathFinder.canPlaceTrace(x, y, layer, grid, boardWidth, boardHeight);
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
            // Площадка остается площадкой, но отмечаем что есть переход
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

// Обработчики кнопок (обертки для слота)
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

