#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "tools/queue_threadsafe.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::queue_threadsafe<char> &q, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void timer_update();
private:
    Ui::MainWindow *ui;
    QTimer *timer;
    std::queue_threadsafe<char> &q_log;
};

#endif // MAINWINDOW_H
