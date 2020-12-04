#include "mainwindow.h"
#include "md-parser.h"

#include <gtkmm/menuitem.h>
#ifdef LEGACY_CXX
#include <experimental/filesystem>
namespace n_fs = ::std::experimental::filesystem;
#else
#include <filesystem>
namespace n_fs = ::std::filesystem;
#endif

MainWindow::MainWindow() : m_vbox(Gtk::ORIENTATION_VERTICAL, 0)
{
    set_title("Browser");
    set_default_size(1000, 800);
    set_position(Gtk::WIN_POS_CENTER);

    // Connect signals
    m_menu.quit.connect(sigc::mem_fun(this, &MainWindow::hide)); /*!< hide main window and therefor closes the app */

    m_vbox.pack_start(m_menu, false, false, 0);

    m_scrolledWindow.add(m_renderArea);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_vbox.pack_end(m_scrolledWindow, true, true, 0);
    add(m_vbox);    
    show_all_children();

    // Just an IPFS test! Fetch a resource from the IPFS network
    // Assuming you already running a IPFS deamon
    network.FetchReadme();

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
