#include "mainwindow.h"
#include "markdown-render.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    MarkdownRender md;
    
    w.setLabel(md.render());
    w.show();
    return a.exec();
}
