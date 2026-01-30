#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QProcess>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleStdout();

private:
    void processLine(const QString &line);

    QTextEdit *rawEdit;
    QTextEdit *extractedEdit;
    QTextEdit *taredEdit;
    QTextEdit *scalingEdit;
    QTextEdit *statusEdit;

    QPushButton *startStopButton;
    QLineEdit *scalingFactorInput;

    QProcess python;
    QByteArray rxBuffer;

    bool readingEnabled = false;
    int scalingFactor = 399835;

    QString deviceArg = "NRF52833_xxAA"; // device argument
};
