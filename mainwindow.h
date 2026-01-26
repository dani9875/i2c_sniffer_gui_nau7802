#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QThread>
#include <ftdi.h>

#include "ftdireader.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onFtdiBytes(const QByteArray &data);
    void processLine(const QByteArray &line);

private:
    // UI
    QTextEdit *rawEdit;
    QTextEdit *extractedEdit;
    QTextEdit *taredEdit;
    QTextEdit *scalingEdit;
    QTextEdit *statusEdit;

    QPushButton *startStopButton;
    QPushButton *tareButton;
    QPushButton *scalingFactorButton;
    QLineEdit *tareInput;
    QLineEdit *scalingFactorInput;

    // State
    bool readingEnabled;
    int scalingFactor;
    int tareValue;

    // FTDI
    ftdi_context *ftdi;

    // Threading
    QThread *readerThread;
    FtdiReader *reader;

    // RX buffer
    QByteArray rxBuffer;
};
