#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QTimer>
#include <ftdi.h>
#include <QLineEdit> 
#include <QPushButton>

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
    QTextEdit *rawEdit;
    QTextEdit *extractedEdit;
    QTextEdit *taredEdit;

    QTimer *timer;
    QTimer *okTimer;
    struct ftdi_context *ftdi;

    QLineEdit *tareInput;
    int tareValue;

    QPushButton *startStopButton;
    bool readingEnabled;
};
