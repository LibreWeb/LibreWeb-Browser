#include "mainwindow.h"

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
    m_menu.reload.connect(sigc::mem_fun(this, &MainWindow::demo)); /*!< reload the page again */
    m_menu.about.connect(sigc::mem_fun(this, &MainWindow::show_about));
    m_about.signal_response().connect(sigc::mem_fun(this, &MainWindow::hide_about));

    m_vbox.pack_start(m_menu, false, false, 0);

    m_scrolledWindow.add(m_renderArea);
    m_scrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_vbox.pack_end(m_scrolledWindow, true, true, 0);
    add(m_vbox);    
    show_all_children();


    demo();
}

void MainWindow::demo()
{
/*
    // From disk
    std::string exePath = n_fs::current_path().string();
    std::string filePath = exePath.append("/../../test.md");
    cmark_node *readDoc = m_file.read(filePath);
    if (readDoc != NULL) {
        m_renderArea.processDocument(readDoc);
        m_file.free(readDoc);
    }
*/
    // From IPFS
    try {
        cmark_node *fetchDoc = m_file.fetch("QmQzhn6hEfbYdCfwzYFsSt3eWpubVKA1dNqsgUwci5vHwq");
        m_renderArea.processDocument(fetchDoc);
        m_file.free(fetchDoc);
    } catch (const std::runtime_error &error) {
        std::cerr << "IPFS Deamon is most likely down: " << error.what() << std::endl;
        // Not found (or any other issue)
        m_renderArea.showMessage("Page not found!", "Detailed error message: " + std::string(error.what()));
    }
}

void MainWindow::show_about()
{
    m_about.run();
}

void MainWindow::hide_about(int response)
{
    m_about.hide();
}
