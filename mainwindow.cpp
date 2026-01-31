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
    scalingEdit = new QTextEdit(this);      // Right: scaled weight
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
        if (ok && v != 0) scalingFactor = v;
    });

    connect(startStopButton, &QPushButton::toggled, this, [this](bool on){
        readingEnabled = on;
        startStopButton->setText(on ? "Stop" : "Start");
    
        if (on) {
            // Disconnect previous signals if any
            python.disconnect();
    
            // Connect error and finished signals
            connect(&python, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
                QString msg;
                switch (error) {
                    case QProcess::FailedToStart: msg = "Python failed to start. Check executable/path."; break;
                    case QProcess::Crashed:       msg = "Python crashed while running."; break;
                    case QProcess::Timedout:      msg = "Python timed out."; break;
                    case QProcess::ReadError:     msg = "Read error from Python."; break;
                    case QProcess::WriteError:    msg = "Write error to Python."; break;
                    case QProcess::UnknownError:  msg = "Unknown Python error."; break;
                }
                extractedEdit->append("[ERROR] " + msg);
                startStopButton->setChecked(false);
                readingEnabled = false;
                startStopButton->setText("Start");
            });
    
            connect(&python, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this, [this](int exitCode, QProcess::ExitStatus status){
                QString msg = QString("[INFO] Python finished with exit code %1").arg(exitCode);
                extractedEdit->append(msg);
                startStopButton->setChecked(false);
                readingEnabled = false;
                startStopButton->setText("Start");
            });
    
            python.start("python3", {"-u", "jlink_helper.py", deviceArg});
    
            // Check immediately if the process failed to start
            if (!python.waitForStarted(1000)) { // wait 1 second
                extractedEdit->append("[ERROR] Python did not start within 1 second.");
                startStopButton->setChecked(false);
                readingEnabled = false;
                startStopButton->setText("Start");
            } else {
                extractedEdit->append("[INFO] Python started successfully with argument: " + deviceArg);
            }
        } else {
            python.terminate();
            if (!python.waitForFinished(1000)) {
                python.kill();
                python.waitForFinished();
            }
            extractedEdit->append("[INFO] Python stopped.");
        }
    });
    

    // --- Labels above the columns ---
    QHBoxLayout *columnLabels = new QHBoxLayout();
    columnLabels->addWidget(new QLabel("RawData"));
    columnLabels->addSpacing(560); // spacing to roughly align with extractedEdit width
    columnLabels->addWidget(new QLabel("Weight (grams)"));
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
    centralLayout->addLayout(columnLabels); // labels on top
    centralLayout->addLayout(mainLayout);   // the two QTextEdits
    centralLayout->addLayout(bottomRow);    // controls at bottom

    QWidget *central = new QWidget(this);
    central->setLayout(centralLayout);
    setCentralWidget(central);

    resize(1200, 700); // adjust overall window width
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
    float grams = 0.0f;

    if (mLoad.hasMatch()) {
        raw = mLoad.captured(1).toInt();
        grams = static_cast<float>(raw) / scalingFactor;

        extractedEdit->append(QString("[%1] %2").arg(ts).arg(raw));
        scalingEdit->append(QString("[%1] %2").arg(ts).arg(grams, 0, 'f', 3));
    }

    // --- State changes (generalized) ---
    int arrowIdx = line.indexOf("-->");
    if (arrowIdx != -1) {
        QString before = line.left(arrowIdx).trimmed();
        QString after  = line.mid(arrowIdx + 3).trimmed(); // +3 to skip the arrow

        // Regex to extract only ALL_CAPS_WITH_UNDERSCORES
        QRegularExpression reState(R"([A-Z_]+)");

        auto beforeMatch = reState.match(before);
        if (beforeMatch.hasMatch())
            before = beforeMatch.captured(0);

        auto afterMatch = reState.match(after);
        if (afterMatch.hasMatch())
            after = afterMatch.captured(0);

        prevStateField->setText(before);
        currStateField->setText(after);
    }

}
