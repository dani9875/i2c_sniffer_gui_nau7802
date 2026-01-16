#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QTimer>
#include <ftdi.h>
#include <QLineEdit> 

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
    QTimer *timer;
    QTimer *okTimer;
    struct ftdi_context *ftdi;

    QLineEdit *tareInput;
    double tareValue;
};
