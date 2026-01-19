#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>

#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      readingEnabled(false)
{
    // --- Raw data window ---
    rawEdit = new QTextEdit(this);
    rawEdit->setReadOnly(true);
    rawEdit->setPlaceholderText("Raw FTDI data");

    // --- Extracted values window ---
    extractedEdit = new QTextEdit(this);
    extractedEdit->setReadOnly(true);
    extractedEdit->setPlaceholderText("Extracted 24-bit values");

    // --- Tared values window ---
    taredEdit = new QTextEdit(this);
    taredEdit->setReadOnly(true);
    taredEdit->setPlaceholderText("Tared values");

    // --- Status pane ---
    statusEdit = new QTextEdit(this);
    statusEdit->setReadOnly(true);

    // --- Start / Stop button ---
    startStopButton = new QPushButton("Start", this);
    startStopButton->setCheckable(true);

    connect(startStopButton, &QPushButton::toggled, this, [this](bool checked) {
        readingEnabled = checked;
        startStopButton->setText(checked ? "Stop" : "Start");
        statusEdit->append(checked ? "Reading started" : "Reading stopped");
    });

    // --- Tare input field ---
    QHBoxLayout *tareLayout = new QHBoxLayout();
    QLabel *tareLabel = new QLabel("Tare:", this);
    tareLayout->addWidget(tareLabel);

    tareInput = new QLineEdit(this);
    tareInput->setPlaceholderText("Enter taring value");
    tareInput->setMaximumWidth(100);
    tareLayout->addWidget(tareInput);

    QPushButton *tareButton = new QPushButton("Set Tare", this);
    tareLayout->addWidget(tareButton);

    connect(tareButton, &QPushButton::clicked, this, [this]() {
        bool ok;
        double value = tareInput->text().toDouble(&ok);
        if (ok) {
            tareValue = value;
            statusEdit->append(QString("Tare set to %1").arg(tareValue));
        } else {
            statusEdit->append("Invalid taring value");
        }
    });

    // --- Layouts ---
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(new QLabel("Raw Data"));
    leftLayout->addWidget(rawEdit);

    QVBoxLayout *middleLayout = new QVBoxLayout();
    middleLayout->addWidget(new QLabel("Extracted Values"));
    middleLayout->addWidget(extractedEdit);

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(new QLabel("Tared Values"));
    rightLayout->addWidget(taredEdit);
    rightLayout->addWidget(new QLabel("Status"));
    rightLayout->addWidget(statusEdit);
    rightLayout->addWidget(startStopButton);
    rightLayout->addLayout(tareLayout);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addLayout(leftLayout, 1);
    layout->addLayout(middleLayout, 1);
    layout->addLayout(rightLayout, 1);

    QWidget *central = new QWidget(this);
    central->setLayout(layout);
    setCentralWidget(central);

    // --- Center window on screen ---
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());

    // // --- Initialize FTDI ---
    // ftdi = ftdi_new();
    // if (!ftdi) {
    //     rawEdit->append("Failed to allocate FTDI context");
    //     return;
    // }

    // if (ftdi_usb_open(ftdi, 0x0403, 0x6001) < 0) {
    //     rawEdit->append(ftdi_get_error_string(ftdi));
    //     return;
    // }

    // ftdi_set_baudrate(ftdi, 9600);

    // --- Timer to poll FTDI ---
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::readFtdiData);
    timer->start(50);

    // --- Timer for colored OK messages ---
    okTimer = new QTimer(this);
    connect(okTimer, &QTimer::timeout, this, [this]() {
        static int colorIndex = 0;
        const QStringList colors = {"green", "red", "blue"};
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

        statusEdit->append(QString("<span style=\"color:%1;\">[%2] OK</span>")
                           .arg(colors[colorIndex])
                           .arg(ts));

        colorIndex = (colorIndex + 1) % colors.size();
    });
    okTimer->start(1000);

    tareValue = 0;
}

MainWindow::~MainWindow()
{
    timer->stop();
    okTimer->stop();
    if (ftdi) {
        ftdi_usb_close(ftdi);
        ftdi_free(ftdi);
    }
}

#define DEBUG_FTDI

void MainWindow::readFtdiData()
{

    if (!readingEnabled)
        return;

    static int byteCount = 0;
    static uint8_t bytes[3];
    int n = 0;

    unsigned char buf[256];

#ifdef DEBUG_FTDI
    static const char *testLines[] = {
        "[2AWA12A[2ARA00N]\n",
        "[2AWA13A[2ARA07N]\n",
        "[2AWA14A[2ARAF8N]\n"
    };
    static size_t testIndex = 0;

    const char *src = testLines[testIndex];
    n = strlen(src);
    memcpy(buf, src, n);

    testIndex = (testIndex + 1) % (sizeof(testLines) / sizeof(testLines[0]));
#else
    n = ftdi_read_data(ftdi, buf, sizeof(buf));
    if (n <= 0) return;
#endif

    QByteArray data(reinterpret_cast<char*>(buf), n);
    QList<QByteArray> lines = data.split('\n');

    for (const QByteArray &line : lines) {
        QString strLine = QString::fromUtf8(line).trimmed();
        if (!strLine.startsWith("[2AW"))
            continue;

        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        rawEdit->append(QString("[%1] %2").arg(timestamp, strLine));

        int rIndex = strLine.indexOf("[2AR");
        if (rIndex == -1 || rIndex + 5 >= strLine.length())
            continue;

        QString byteStr = strLine.mid(rIndex + 4, 2);
        bool ok;
        uint8_t value = byteStr.toUInt(&ok, 16);
        if (!ok) continue;

        if (byteCount < 3)
            bytes[byteCount++] = value;

        if (byteCount == 3) {
            int32_t result = (bytes[0] << 16) |
                             (bytes[1] << 8)  |
                              bytes[2];

            QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            extractedEdit->append(QString("[%1] %2").arg(ts).arg(result));

            int tared = result - tareValue;
            taredEdit->append(QString("[%1] %2").arg(ts).arg(tared));

            byteCount = 0;
        }
    }
}
