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

    // We will not use TextEdit, it does only support HTML (and markdown, but we don't want to use the built-in parser).
    // Instead, we can try QPainter in Qt or use a 2D engine (using ttf, glyphs atlas, render bitmap text).
    textEdit = new QTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textEdit);
}

void MainWindow::setOutput(const QString& text)
{
    textEdit->setHtml(text);
}
