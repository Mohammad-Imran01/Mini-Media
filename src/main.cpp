#include "gui/mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QString>
#include <QStyle>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << a.style()->name();

    MainWindow w;

    w.show();
    return a.exec();
}
