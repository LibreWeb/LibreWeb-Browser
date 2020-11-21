#include "mainwindow.h"
#include "scene.h"
#include "md-parser.h"
#include "qt-renderer.h"

#include <chrono>
#include <iostream>


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

    QMenu fileMenu("File");
    fileMenu.addAction("Open...");
    fileMenu.addAction("New...");
    fileMenu.addAction("Exit");

    menuBar()->addMenu(&fileMenu);

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

MainWindow::~MainWindow()
{
    delete scene;
    delete view;
    delete textEdit;
    delete parser;
    delete renderer;
    delete menuBar();
}

void MainWindow::setupParser()
{
    parser = new Parser();
    renderer = new QtRenderer();
    renderer->setScene(scene);

    std::string exePath = n_fs::current_path().string();
    std::string htmlOutput = "";
    std::string filePath = exePath.append("/../../test.md");
    printf("Path: %s\n", filePath.c_str());

    cmark_node *root_node = parser->parseFile(filePath);
    if (root_node != NULL) {
        htmlOutput = parser->renderHTML(root_node);

        typedef std::chrono::high_resolution_clock Time;
        typedef std::chrono::milliseconds ms;
        typedef std::chrono::duration<float> fsec;
        auto t0 = Time::now();

        // Render AST to scene
        renderer->renderDocument(root_node);

        auto t1 = Time::now();
        fsec fs = t1 - t0;
        ms d = std::chrono::duration_cast<ms>(fs);
        std::cout << "My render: " << d.count() << " ms" << std::endl;

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
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;
    
    auto htmlStart = Time::now();

    textEdit->setHtml(text);

    auto htmlEnd = Time::now();
    fsec diff = htmlEnd - htmlStart;
    ms htmlDuration = std::chrono::duration_cast<ms>(diff);
    std::cout << "HTML: " << htmlDuration.count() << " ms" << std::endl;
}
