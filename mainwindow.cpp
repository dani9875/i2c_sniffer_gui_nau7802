#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QLabel>
#include <QScreen>
#include <QGuiApplication>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // --- Main displays ---
    rawEdit = new QTextEdit(this);          // Top-left: combined RTT + state
    extractedEdit = new QTextEdit(this);    // Right: extracted
    scalingEdit = new QTextEdit(this);      // Right: scaled weight
    prevStateField = new QLineEdit(this);   // Bottom-left row: prev state
    currStateField = new QLineEdit(this);   // Bottom-left row: current state
    taredEdit = new QTextEdit(this);        // Bottom-left row: current weight

    rawEdit->setReadOnly(true);
    extractedEdit->setReadOnly(true);
    scalingEdit->setReadOnly(true);
    taredEdit->setReadOnly(true);
    prevStateField->setReadOnly(true);
    currStateField->setReadOnly(true);

    prevStateField->setMaximumWidth(150);
    currStateField->setMaximumWidth(150);
    taredEdit->setMaximumWidth(150);

    // --- Start/Stop button and scaling ---
    startStopButton = new QPushButton("Start", this);
    startStopButton->setCheckable(true);
    scalingFactorInput = new QLineEdit(QString::number(scalingFactor), this);
    connect(scalingFactorInput, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        int v = scalingFactorInput->text().toInt(&ok);
        if (ok && v != 0) scalingFactor = v;
    });

    connect(startStopButton, &QPushButton::toggled, this, [this](bool on){
        readingEnabled = on;
        startStopButton->setText(on ? "Stop" : "Start");
        if (on) {
            python.start("python3", {"-u", "test1.py", deviceArg});
            rawEdit->append("Python started with argument: " + deviceArg);
        } else {
            python.terminate();
            python.waitForFinished(1000);
            rawEdit->append("Python stopped");
        }
    });

    // --- Layouts ---
    QHBoxLayout *stateRow = new QHBoxLayout();
    stateRow->addWidget(new QLabel("Prev State:"));
    stateRow->addWidget(prevStateField);
    stateRow->addWidget(new QLabel("Curr State:"));
    stateRow->addWidget(currStateField);
    stateRow->addWidget(new QLabel("Current Weight:"));
    stateRow->addWidget(taredEdit);

    QVBoxLayout *leftBlock = new QVBoxLayout();
    leftBlock->addWidget(rawEdit);   // Top large RTT display
    leftBlock->addLayout(stateRow);  // Bottom row with prev/curr states and current weight

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addLayout(leftBlock);
    mainLayout->addWidget(extractedEdit);
    mainLayout->addWidget(scalingEdit);

    QVBoxLayout *controls = new QVBoxLayout();
    controls->addLayout(mainLayout);
    QHBoxLayout *bottomControls = new QHBoxLayout();
    bottomControls->addWidget(startStopButton);
    bottomControls->addWidget(new QLabel("Scaling:"));
    bottomControls->addWidget(scalingFactorInput);
    controls->addLayout(bottomControls);

    QWidget *central = new QWidget(this);
    central->setLayout(controls);
    setCentralWidget(central);

    resize(1400, 700);
    move(QGuiApplication::primaryScreen()->geometry().center() - rect().center());

    connect(&python, &QProcess::readyReadStandardOutput, this, &MainWindow::handleStdout);
}

MainWindow::~MainWindow()
{
    python.terminate();
    python.waitForFinished(500);
}

void MainWindow::handleStdout()
{
    rxBuffer.append(python.readAllStandardOutput());

    int idx;
    while ((idx = rxBuffer.indexOf('\n')) != -1) {
        QByteArray line = rxBuffer.left(idx);
        rxBuffer.remove(0, idx + 1);
        processLine(QString::fromUtf8(line).trimmed());
    }
}

void MainWindow::processLine(const QString &line)
{
    if (!readingEnabled) return;

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // --- Load Cell Reading ---
    static QRegularExpression reLoad(R"(Load Cell Reading:\s*(-?\d+))");
    auto mLoad = reLoad.match(line);
    int raw = 0;
    int tared = 0;
    float grams = 0.0f;

    if (mLoad.hasMatch()) {
        raw = mLoad.captured(1).toInt();
        tared = raw;  // assuming already tared
        grams = static_cast<float>(tared) / scalingFactor;

        rawEdit->append(QString("[%1] %2").arg(ts, line));
        extractedEdit->append(QString("[%1] %2").arg(ts).arg(raw));
        scalingEdit->append(QString("[%1] %2").arg(ts).arg(grams, 0, 'f', 3));
        taredEdit->setText(QString::number(tared));
    }

    // --- State changes (generalized) ---
    int arrowIdx = line.indexOf("-->");
    if (arrowIdx != -1) {
        QString before = line.left(arrowIdx).trimmed();
        QString after  = line.mid(arrowIdx + 3).trimmed(); // +3 to skip the arrow
        printf("State change detected: %s --> %s\n",
               before.toUtf8().constData(),
               after.toUtf8().constData());
        prevStateField->setText(before);
        currStateField->setText(after);
    }
}

