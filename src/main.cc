
#include "md-parser.h"
#include "mainwindow.h"

#include <filesystem>
#include <QApplication>
#include <QTextDocument>
#include <QString>

int main(int argc, char *argv[])
{
    Parser parser; 
    
    std::string exePath = std::filesystem::current_path().string();
    std::string htmlOutput, myOutput = "";
    std::string filePath = exePath.append("/../../test.md");
    printf("Path: %s\n", filePath.c_str());

    cmark_node *root_node = parser.parseFile(filePath);
    if (root_node != NULL) {
        htmlOutput = parser.renderHTML(root_node);
        myOutput = parser.renderMyLayout(root_node);

        cmark_node_free(root_node);
    }

    QApplication app(argc, argv);

    MainWindow window;
    window.setOutputToTextEdit(QString::fromStdString(htmlOutput));
    window.drawOutputToScene(QString::fromStdString(myOutput));
    window.show();
    return app.exec();
}
