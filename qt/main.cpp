#include "mainwindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("fusion"));

    QFont qf;
    qf.setFamily("黑体");
    qf.setPointSize(12);

    MainWindow w;
    w.show();
    return a.exec();
}
