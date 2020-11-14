#include "mainwindow.h"

#include <QVBoxLayout>
#include <QMenuBar>
#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QSizePolicy>

MainWindow::MainWindow()
{
    resize(600, 400);
    setWindowTitle("Browser");

    QMenu *fileMenu = new QMenu("File");
    fileMenu->addAction("New");
    fileMenu->addAction("Exit");

    menuBar()->addMenu(fileMenu);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(5, 5, 5, 5);

    // TextEdit supports markdown input as QString (but we don't want to use Qt's implemention), with Qt version 5.14
    // TextEdit supports QTextDocument, but it only supports HTML as 'rich text'...
    // Bottom-line: If we directly want to render rich text, we need to create our own version of TextEdit and/or create a 2D layout.
    // Meaning we can just better use a 2D engine after all.

    // We want full control, reading TrueType fonts, creating a Font/glyph Atlas. Calculate bitmap and render our text on the screen.
    textEdit = new QTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textEdit);
}

void MainWindow::setOutput(const QString& text)
{
    textEdit->setText(text);
}
