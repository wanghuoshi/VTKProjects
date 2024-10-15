#include "MPRReader.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MPRReader w;
    w.show();
    return a.exec();
}
