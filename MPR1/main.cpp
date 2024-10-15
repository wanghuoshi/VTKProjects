#include "MPRViewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MPRViewer w;
    w.show();
    return a.exec();
}
