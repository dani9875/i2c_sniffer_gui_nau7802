#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QTimer>
#include <ftdi.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void readFtdiData();

private:
    QTextEdit *textEdit;
    QTextEdit *statusEdit;
    struct ftdi_context *ftdi;
    QTimer *timer;
    QTimer *okTimer;
};
