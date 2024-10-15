#include "SurfaceRendering.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SurfaceRendering w;
    w.show();
    return a.exec();
}
