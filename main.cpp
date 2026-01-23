#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Отключаем предупреждения о геометрии
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    // Для старых версий Qt используйте более простой подход
    #if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    #endif

    // Альтернативно, можно попробовать отключить DPI scaling
    QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    QApplication a(argc, argv);

    // Устанавливаем стиль для исправления проблем с диалоговыми окнами
    QApplication::setStyle("fusion");

    MainWindow w;
    w.show();

    return a.exec();
}
