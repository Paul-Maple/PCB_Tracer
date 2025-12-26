#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Отключаем предупреждения о геометрии
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Если нужно отключить дополнительные предупреждения:
    // QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
