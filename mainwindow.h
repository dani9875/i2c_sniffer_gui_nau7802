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
    QTextEdit *scalingEdit;

    QTimer *timer;
    QTimer *okTimer;
    struct ftdi_context *ftdi;

    QLineEdit *tareInput;
    QPushButton *tareButton;
    int tareValue;

    QPushButton *startStopButton;

    QLineEdit *scalingFactorInput; // user can edit scaling factor
    QPushButton *scalingFactorButton;
    int scalingFactor;

    bool readingEnabled;
};
