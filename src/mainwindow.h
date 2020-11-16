#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QTextEdit;
class QGraphicsView;
class Scene;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void setOutputToTextEdit(const QString& text);
    void drawOutputToScene(const QString& text);

//private slots:
//    void newFile();

private:
    QGraphicsView *view;
    Scene *scene;
    QTextEdit *textEdit;

    // void resizeEvent(QResizeEvent *);
};

#endif