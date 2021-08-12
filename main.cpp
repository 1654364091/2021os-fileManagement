#include "filesystem.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    fileSystem w;
    w.show();
    return a.exec();
}
