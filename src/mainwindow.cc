#include "mainwindow.h"
#include "scene.h"
#include "md-parser.h"
#include "md-render.h"

#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

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

    scene = new Scene(this);
    view = new QGraphicsView(scene);
    //view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAlignment(Qt::Alignment::enum_type::AlignTop|Qt::Alignment::enum_type::AlignLeft);
    layout->addWidget(view);

    // We will not use TextEdit, it does only support HTML (and markdown, but we don't want to use the built-in parser).
    // Instead, we can try QPainter in Qt or use a 2D engine (using ttf, glyphs atlas, render bitmap text).
    textEdit = new QTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textEdit);

    // Setup parser & renderer
    setupParser();
}

void MainWindow::setupParser()
{
    parser = new Parser();
    renderer = new Renderer(scene);

    std::string exePath = n_fs::current_path().string();
    std::string htmlOutput = "";
    std::string filePath = exePath.append("/../../test.md");
    printf("Path: %s\n", filePath.c_str());

    cmark_node *root_node = parser->parseFile(filePath);
    if (root_node != NULL) {
        htmlOutput = parser->renderHTML(root_node);

        // Render AST to scene
        renderer->renderDocument(root_node);

        cmark_node_free(root_node);
    }

    setOutputToTextEdit(QString::fromStdString(htmlOutput));
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
