```cpp
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
    extractedEdit = new QTextEdit(this);    // Center: extracted
    scalingEdit   = new QTextEdit(this);    // Right: scaled weight
    prevStateField = new QLineEdit(this);   // Bottom row: prev state
    currStateField = new QLineEdit(this);   // Bottom row: current state

    extractedEdit->setReadOnly(true);
    scalingEdit->setReadOnly(true);
    prevStateField->setReadOnly(true);
    currStateField->setReadOnly(true);

    // --- Adjust widths ---
    extractedEdit->setMinimumWidth(600);
    scalingEdit->setMinimumWidth(300);
    prevStateField->setMaximumWidth(550);
    currStateField->setMaximumWidth(550);

    // --- Start/Stop button and scaling ---
    startStopButton = new QPushButton("Start", this);
    startStopButton->setCheckable(true);

    scalingFactorInput = new QLineEdit(QString::number(scalingFactor), this);
    scalingFactorInput->setMaximumWidth(80);

    connect(scalingFactorInput, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        int v = scalingFactorInput->text().toInt(&ok);
        if (ok && v != 0)
            scalingFactor = v;
    });

    connect(startStopButton, &QPushButton::toggled, this, [this](bool on){
        readingEnabled = on;
        startStopButton->setText(on ? "Stop" : "Start");

        if (on) {
            python.start("python3", {"-u", "jlink_helper.py", deviceArg});
        } else {
            python.terminate();
            python.waitForFinished(1000);
        }
    });

    // --- Labels above the columns ---
    QHBoxLayout *columnLabels = new QHBoxLayout();
    columnLabels->addWidget(new QLabel("RawData"));
    columnLabels->addSpacing(560);
    columnLabels->addWidget(new QLabel("Weight (grams + phase)"));
    columnLabels->addStretch();

    // --- Main horizontal layout ---
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(extractedEdit);
    mainLayout->addWidget(scalingEdit);

    // --- Bottom row layout ---
    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->addWidget(startStopButton);
    bottomRow->addWidget(new QLabel("Scaling:"));
    bottomRow->addWidget(scalingFactorInput);
    bottomRow->addSpacing(20);
    bottomRow->addWidget(new QLabel("Prev State:"));
    bottomRow->addWidget(prevStateField);
    bottomRow->addWidget(new QLabel("Curr State:"));
    bottomRow->addWidget(currStateField);
    bottomRow->addStretch();

    // --- Central vertical layout ---
    QVBoxLayout *centralLayout = new QVBoxLayout();
    centralLayout->addLayout(columnLabels);
    centralLayout->addLayout(mainLayout);
    centralLayout->addLayout(bottomRow);

    QWidget *central = new QWidget(this);
    central->setLayout(centralLayout);
    setCentralWidget(central);

    resize(1200, 700);
    move(QGuiApplication::primaryScreen()->geometry().center() - rect().center());

    connect(&python, &QProcess::readyReadStandardOutput,
            this, &MainWindow::handleStdout);
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
    if (!readingEnabled)
        return;

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // --- Load Cell Reading ---
    static QRegularExpression reLoad(R"(Load Cell Reading:\s*(-?\d+))");
    auto mLoad = reLoad.match(line);

    if (mLoad.hasMatch()) {
        int raw = mLoad.captured(1).toInt();
        float grams = static_cast<float>(raw) / scalingFactor;

        extractedEdit->append(
            QString("[%1] %2")
                .arg(ts)
                .arg(raw)
        );

        if (!currentPhase.isEmpty()) {
            scalingEdit->append(
                QString("[%1] %2 g  (%3)")
                    .arg(ts)
                    .arg(grams, 0, 'f', 3)
                    .arg(currentPhase)
            );
        } else {
            scalingEdit->append(
                QString("[%1] %2 g")
                    .arg(ts)
                    .arg(grams, 0, 'f', 3)
            );
        }
    }

    // --- Can Height ---
    static QRegularExpression reCanHeight(R"(Can Height:\s*(-?\d+))");
    auto mCanHeight = reCanHeight.match(line);

    if (mCanHeight.hasMatch()) {
        extractedEdit->append(
            QString("[%1] Can Height: %2")
                .arg(ts)
                .arg(mCanHeight.captured(1))
        );
    }

    // --- Carousel stall values ---
    static QRegularExpression reStall(R"(Carousel stall values:\s*(-?\d+),\s*(-?\d+),\s*(-?\d+))");
    auto mStall = reStall.match(line);

    if (mStall.hasMatch()) {
        int load          = mStall.captured(1).toInt();
        int stallLoad      = mStall.captured(2).toInt();
        int stepsPerSec    = mStall.captured(3).toInt();

        extractedEdit->append(
            QString("[%1] Stall load=%2 stall_load=%3 steps/s=%4")
                .arg(ts)
                .arg(load)
                .arg(stallLoad)
                .arg(stepsPerSec)
        );
    }

    // --- State changes ---
    int arrowIdx = line.indexOf("-->");
    if (arrowIdx != -1) {
        QString before = line.left(arrowIdx).trimmed();
        QString after  = line.mid(arrowIdx + 3).trimmed();

        QRegularExpression reState(R"([A-Z_]+)");

        auto beforeMatch = reState.match(before);
        if (beforeMatch.hasMatch())
            before = beforeMatch.captured(0);

        auto afterMatch = reState.match(after);
        if (afterMatch.hasMatch())
            after = afterMatch.captured(0);

        prevStateField->setText(before);
        currStateField->setText(after);

        currentPhase = after;
    }
}
```