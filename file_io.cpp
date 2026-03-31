#include "file_io.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

bool loadBoardData(const QString& filename, BoardData& data) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for reading:" << filename;
        return false;
    }

    data = BoardData();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(' ', QString::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        QString key = parts[0].toUpper();
        if (key == "BOARD_WIDTH" && parts.size() >= 2) {
            data.width = parts[1].toInt();
        } else if (key == "BOARD_HEIGHT" && parts.size() >= 2) {
            data.height = parts[1].toInt();
        } else if (key == "LAYER_COUNT" && parts.size() >= 2) {
            data.layerCount = parts[1].toInt();
        } else if (key == "PAD" && parts.size() >= 5) {
            PadData pad;
            pad.id = parts[1].toInt();
            pad.x = parts[2].toInt();
            pad.y = parts[3].toInt();
            pad.name = parts[4];
            data.pads.append(pad);
        } else if (key == "OBSTACLE" && parts.size() >= 4) {
            ObstacleData obs;
            obs.x = parts[1].toInt();
            obs.y = parts[2].toInt();
            obs.layer = parts[3].toInt();
            data.obstacles.append(obs);
        } else if (key == "CONNECTION" && parts.size() >= 3) {
            ConnectionData conn;
            conn.fromPadId = parts[1].toInt();
            conn.toPadId = parts[2].toInt();
            data.connections.append(conn);
        } else if (key == "END") {
            break;
        } else {
            qDebug() << "Unknown line in file:" << line;
        }
    }

    file.close();
    return true;
}

bool saveBoardData(const QString& filename, const BoardData& data) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for writing:" << filename;
        return false;
    }

    QTextStream out(&file);
    out << "# PCB_Trace data file\n";
    out << "BOARD_WIDTH " << data.width << "\n";
    out << "BOARD_HEIGHT " << data.height << "\n";
    out << "LAYER_COUNT " << data.layerCount << "\n";

    for (const PadData& pad : data.pads) {
        out << "PAD " << pad.id << " " << pad.x << " " << pad.y << " " << pad.name << "\n";
    }

    for (const ObstacleData& obs : data.obstacles) {
        out << "OBSTACLE " << obs.x << " " << obs.y << " " << obs.layer << "\n";
    }

    for (const ConnectionData& conn : data.connections) {
        out << "CONNECTION " << conn.fromPadId << " " << conn.toPadId << "\n";
    }

    out << "END\n";
    file.close();
    return true;
}
