#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "tools/queue_threadsafe.h"
#include "asuit_protocol.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(std::queue_threadsafe<char> &q, Flasher *flasher, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void timer_update();
    void on_btn_upd_ctrl_clicked();
    void on_btn_upd_head_clicked();
    void on_btn_upd_tail_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    std::queue_threadsafe<char> &q_log;
    void save_paths();
    Flasher *flasher;
};

#endif // MAINWINDOW_H
