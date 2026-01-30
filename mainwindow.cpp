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
    rawEdit = new QTextEdit(this);
    extractedEdit = new QTextEdit(this);
    taredEdit = new QTextEdit(this);
    scalingEdit = new QTextEdit(this);
    statusEdit = new QTextEdit(this);

    for (auto e : {rawEdit, extractedEdit, taredEdit, scalingEdit, statusEdit})
        e->setReadOnly(true);

    startStopButton = new QPushButton("Start", this);
    startStopButton->setCheckable(true);

    connect(startStopButton, &QPushButton::toggled, this, [this](bool on) {
        readingEnabled = on;
        startStopButton->setText(on ? "Stop" : "Start");

        if (on) {
            python.start("python3", {"-u", "test1.py", deviceArg});
            statusEdit->append("Python started with argument: " + deviceArg);
        } else {
            python.terminate();
            python.waitForFinished(1000);
            statusEdit->append("Python stopped");
        }
    });

    scalingFactorInput = new QLineEdit(QString::number(scalingFactor), this);

    connect(scalingFactorInput, &QLineEdit::editingFinished, this, [this]() {
        bool ok;
        int v = scalingFactorInput->text().toInt(&ok);
        if (ok && v != 0)
            scalingFactor = v;
    });

    QHBoxLayout *controls = new QHBoxLayout();
    controls->addWidget(startStopButton);
    controls->addWidget(new QLabel("Scaling:"));
    controls->addWidget(scalingFactorInput);

    QHBoxLayout *top = new QHBoxLayout();
    top->addWidget(rawEdit);
    top->addWidget(extractedEdit);
    top->addWidget(taredEdit);
    top->addWidget(scalingEdit);
    top->addWidget(statusEdit);

    QVBoxLayout *main = new QVBoxLayout();
    main->addLayout(top);
    main->addLayout(controls);

    QWidget *central = new QWidget(this);
    central->setLayout(main);
    setCentralWidget(central);

    resize(1200, 600);
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

    static QRegularExpression re(R"(Load Cell Reading:\s*(-?\d+))");
    auto m = re.match(line);
    if (!m.hasMatch())
        return;

    int raw = m.captured(1).toInt();
    int tared = raw;
    float grams = static_cast<float>(tared) / scalingFactor;

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    rawEdit->append(QString("[%1] %2").arg(ts, line));
    extractedEdit->append(QString("[%1] %2").arg(ts).arg(raw));
    taredEdit->append(QString("[%1] %2").arg(ts).arg(tared));
    scalingEdit->append(QString("[%1] %2").arg(ts).arg(grams, 0, 'f', 3));
}
