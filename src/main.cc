#include "mainwindow.h"
#include "markdown-render.h"

#include <QApplication>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    MarkdownRender md;

    QString output = QString::fromStdString(md.render());
    w.setLabel(output);
    w.show();
    return a.exec();
}
