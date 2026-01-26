#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QLabel>
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      readingEnabled(false),
      scalingFactor(399835),
      tareValue(2625000),
      ftdi(nullptr),
      readerThread(nullptr),
      reader(nullptr)
{
    rawEdit = new QTextEdit(this);
    extractedEdit = new QTextEdit(this);
    taredEdit = new QTextEdit(this);
    scalingEdit = new QTextEdit(this);
    statusEdit = new QTextEdit(this);

    rawEdit->setReadOnly(true);
    extractedEdit->setReadOnly(true);
    taredEdit->setReadOnly(true);
    scalingEdit->setReadOnly(true);
    statusEdit->setReadOnly(true);

    startStopButton = new QPushButton("Start", this);
    startStopButton->setCheckable(true);

    connect(startStopButton, &QPushButton::toggled, this, [this](bool checked) {
        readingEnabled = checked;
        startStopButton->setText(checked ? "Stop" : "Start");
    });

    tareButton = new QPushButton("Set Tare:", this);
    tareInput = new QLineEdit(QString::number(tareValue), this);

    connect(tareButton, &QPushButton::clicked, this, [this]() {
        bool ok;
        int v = tareInput->text().toInt(&ok);
        if (ok) tareValue = v;
    });

    scalingFactorInput = new QLineEdit(QString::number(scalingFactor), this);
    scalingFactorButton = new QPushButton("Apply", this);

    connect(scalingFactorButton, &QPushButton::clicked, this, [this]() {
        bool ok;
        int v = scalingFactorInput->text().toInt(&ok);
        if (ok && v != 0) scalingFactor = v;
    });

    QHBoxLayout *controls = new QHBoxLayout();
    controls->addWidget(startStopButton);
    controls->addWidget(tareButton);
    controls->addWidget(tareInput);
    controls->addWidget(new QLabel("Scaling:"));
    controls->addWidget(scalingFactorInput);
    controls->addWidget(scalingFactorButton);

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

    // ---- FTDI init ----
    ftdi = ftdi_new();
    if (!ftdi) return;

    if (ftdi_usb_open(ftdi, 0x0403, 0x6001) < 0) return;
    ftdi_set_baudrate(ftdi, 921600);

    readerThread = new QThread(this);
    reader = new FtdiReader(ftdi);

    reader->moveToThread(readerThread);

    connect(readerThread, &QThread::started, reader, &FtdiReader::start);
    connect(readerThread, &QThread::finished, reader, &QObject::deleteLater);
    connect(this, &MainWindow::destroyed, reader, &FtdiReader::stop);
    connect(reader, &FtdiReader::bytesReceived, this, &MainWindow::onFtdiBytes);

    readerThread->start();
}

MainWindow::~MainWindow()
{
    if (readerThread) {
        reader->stop();
        readerThread->quit();
        readerThread->wait();
    }

    if (ftdi) {
        ftdi_usb_close(ftdi);
        ftdi_free(ftdi);
    }
}

void MainWindow::onFtdiBytes(const QByteArray &data)
{
    if (!readingEnabled)
        return;

    rxBuffer.append(data);

    int idx;
    while ((idx = rxBuffer.indexOf('\n')) != -1) {
        QByteArray line = rxBuffer.left(idx);
        rxBuffer.remove(0, idx + 1);
        processLine(line);
    }
}

void MainWindow::processLine(const QByteArray &line)
{
    static enum { EXPECT_12, EXPECT_13, EXPECT_14 } state = EXPECT_12;
    static uint8_t bytes[3];
    static QString tripletTimestamp;

    QString strLine = QString::fromUtf8(line).trimmed();
    if (!strLine.startsWith("[2AWA"))
        return;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    rawEdit->append(QString("[%1] %2").arg(timestamp, strLine));

    int rIndex = strLine.indexOf("[2AR");
    if (rIndex == -1 || rIndex + 5 >= strLine.length()) {
        state = EXPECT_12;
        return;
    }

    bool ok;
    uint8_t value = strLine.mid(rIndex + 5, 2).toUInt(&ok, 16);
    if (!ok) {
        state = EXPECT_12;
        return;
    }

    if (state == EXPECT_12 && strLine.startsWith("[2AWA12A")) {
        bytes[0] = value;
        tripletTimestamp = timestamp;
        state = EXPECT_13;
    }
    else if (state == EXPECT_13 && strLine.startsWith("[2AWA13A")) {
        bytes[1] = value;
        state = EXPECT_14;
    }
    else if (state == EXPECT_14 && strLine.startsWith("[2AWA14A")) {
        bytes[2] = value;

        uint32_t result =
            (bytes[0] << 16) |
            (bytes[1] << 8) |
             bytes[2];

        result *= 1000;

        extractedEdit->append(QString("[%1] %2").arg(tripletTimestamp).arg(result));

        int tared = result - tareValue;
        taredEdit->append(QString("[%1] %2").arg(tripletTimestamp).arg(tared));

        float grams = static_cast<float>(tared) / scalingFactor;
        scalingEdit->append(QString("[%1] %2").arg(tripletTimestamp).arg(grams, 0, 'f', 3));

        state = EXPECT_12;
    }
    else {
        state = EXPECT_12;
    }
}
