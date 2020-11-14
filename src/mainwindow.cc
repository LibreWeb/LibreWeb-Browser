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

    textEdit = new QTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textEdit);
}

void MainWindow::setOutput(const QString& text)
{
    textEdit->setText(text);
}
