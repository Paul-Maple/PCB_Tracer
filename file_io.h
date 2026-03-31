#ifndef FILE_IO_H
#define FILE_IO_H

#include <QString>
#include <QList>

struct PadData {
    int id;
    int x;
    int y;
    QString name;
};

struct ObstacleData {
    int x;
    int y;
    int layer;
};

struct ConnectionData {
    int fromPadId;
    int toPadId;
};

struct BoardData {
    int width;
    int height;
    int layerCount;
    QList<PadData> pads;
    QList<ObstacleData> obstacles;
    QList<ConnectionData> connections;
};

bool loadBoardData(const QString& filename, BoardData& data);
bool saveBoardData(const QString& filename, const BoardData& data);

#endif // FILE_IO_H