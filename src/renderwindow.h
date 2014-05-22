#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <QMainWindow>
#include <QGLWidget>

namespace Ui {
class renderWindow;
}

class renderWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit renderWindow(QWidget *parent = 0);
    ~renderWindow();


private:
    Ui::renderWindow *ui;
    QGLWidget *grabbedWidget;
    void closeEvent(QCloseEvent *);
signals:
    void passWidget(QGLWidget *widget);

public slots:
    void receiveWidget(QGLWidget *newglview);
};

#endif // RENDERWINDOW_H
