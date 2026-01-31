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

    // --- Display widgets ---
    QTextEdit *rawEdit;          // Top-left: combined RTT + state
    QTextEdit *extractedEdit;    // Right: extracted values
    QTextEdit *scalingEdit;      // Right: scaled weight
    QTextEdit *taredEdit;        // Bottom-left: current weight
    QLineEdit *prevStateField;   // Bottom-left: previous state
    QLineEdit *currStateField;   // Bottom-left: current state

    // --- Controls ---
    QPushButton *startStopButton;
    QLineEdit *scalingFactorInput;

    // --- Process and buffer ---
    QProcess python;
    QByteArray rxBuffer;

    // --- State ---
    bool readingEnabled = false;
    int scalingFactor = 399835;
    QString deviceArg = "NRF52833_xxAA"; // device argument
};
