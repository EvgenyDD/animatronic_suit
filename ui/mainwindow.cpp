#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"

MainWindow::MainWindow(std::queue_threadsafe<char> &q, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    q_log(q)
{
    ui->setupUi(this);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_update()));
    timer->start(20);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timer_update()
{
    QString log;
    char v;
    while(q_log.try_pop(v)) log.append(v);
    if(log.size())
    {
//        qDebug() << log;
        ui->logBrowser->append(log);
    }
}
