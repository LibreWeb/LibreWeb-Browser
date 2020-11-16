#include "mainwindow.h"
#include "scene.h"

#include <QVBoxLayout>
#include <QMenuBar>
#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QSizePolicy>
#include <QGraphicsView>
#include <QRectF>
#include <QGraphicsTextItem>

MainWindow::MainWindow()
{
    resize(600, 400);
    setWindowTitle("Browser");

    QMenu *fileMenu = new QMenu("File");
    fileMenu->addAction("Open...");
    fileMenu->addAction("New...");
    fileMenu->addAction("Exit");

    menuBar()->addMenu(fileMenu);

    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(5, 5, 5, 5);

    scene = new Scene();
    scene->setSceneRect(QRectF(0, 0, 200, 180));
    view = new QGraphicsView(scene);
    //view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAlignment(Qt::Alignment::enum_type::AlignLeft);
    layout->addWidget(view);

    // We will not use TextEdit, it does only support HTML (and markdown, but we don't want to use the built-in parser).
    // Instead, we can try QPainter in Qt or use a 2D engine (using ttf, glyphs atlas, render bitmap text).
    textEdit = new QTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textEdit);
}

/**
 * Can be used for resizing the scene
void MainWindow::resizeEvent(QResizeEvent *) {
    QRectF bounds = scene->itemsBoundingRect();
    bounds.setWidth(bounds.width()*0.9);         // to tighten-up margins
    bounds.setHeight(bounds.height()*0.9);       // same as above
    view->fitInView(bounds, Qt::KeepAspectRatio);
    view->centerOn(0, 0);
}*/

/**
 * Example of adding text (plaintext or html) to text edit
 */
void MainWindow::setOutputToTextEdit(const QString& text)
{
    textEdit->setHtml(text);
}

// TODO: Create a new Render class for converting AST and render it to the scene, by calculating the positions, etc.
//       So the input parameter will be a root node, render the right Qt objects with their positions and settings, and drop in on the scene.
//       Basically cmark parse and Qt are comming togther.
void MainWindow::drawOutputToScene(const QString& text)
{
    QGraphicsTextItem *textItem = new QGraphicsTextItem(text);
    QFont font;
    font.setBold(true);
    font.setFamily("Open Sans"); // Arial
    textItem->setFont(font);
    textItem->setPos(10, 40);
    textItem->setTextWidth(200);

    scene->addItem(textItem);
}
