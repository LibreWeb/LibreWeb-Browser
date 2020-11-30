#include "mainwindow.h"
#include "md-parser.h"

#include <chrono>
#include <iostream>
#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

MainWindow::MainWindow()
{
    set_title("Browser");
    set_default_size(800, 600);
    set_position(Gtk::WIN_POS_CENTER_ALWAYS);

    add(m_scrolledWindow);

    m_scrolledWindow.add(m_renderArea);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    
    show_all_children();

    // Setup parser
    setupParser();
}

MainWindow::~MainWindow()
{
    delete parser;
}

void MainWindow::setupParser()
{
    parser = new Parser();

    std::string exePath = n_fs::current_path().string();
    std::string filePath = exePath.append("/../../test.md");
    printf("Path: %s\n", filePath.c_str());

    cmark_node *root_node = parser->parseFile(filePath);
    if (root_node != NULL) {
        /*std::string html = parser->renderHTML(root_node);
        printf("HTML %s\n\n", html.c_str());*/

        // process AST, which can then be drawed on render/drawing area
        m_renderArea.processDocument(root_node);
        cmark_node_free(root_node);
    }
}
