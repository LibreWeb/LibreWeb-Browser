#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Scene;
class Parser;
class QtRenderer;
QT_BEGIN_NAMESPACE
class QAction;
class QTextEdit;
class QGraphicsView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void setOutputToTextEdit(const QString& text);

//private slots:
//    void newFile();

private:
    QGraphicsView *view;
    QTextEdit *textEdit;
    Scene *scene;
    Parser *parser;
    QtRenderer *renderer;

    void setupParser();
    // void resizeEvent(QResizeEvent *);
};

#endif