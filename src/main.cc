
#include "markdown-render.h"
#include "mainwindow.h"

#include <QApplication>
#include <QTextDocument>
#include <QString>

int main(int argc, char *argv[])
{
    MarkdownRender md;
    QString output = QString::fromStdString(md.render());

    QApplication app(argc, argv);

    MainWindow window;
    window.setOutput(output);
    window.show();
    return app.exec();
}
