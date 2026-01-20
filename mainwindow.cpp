#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      readingEnabled(false),
      scalingFactor(399835),
      tareValue(2625000)
{
    // --- Raw data window ---
    rawEdit = new QTextEdit(this);
    rawEdit->setReadOnly(true);
    rawEdit->setPlaceholderText("Raw FTDI data");
    rawEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rawEdit->setMinimumHeight(300);
    rawEdit->setMinimumWidth(400);

    // --- Extracted values window ---
    extractedEdit = new QTextEdit(this);
    extractedEdit->setReadOnly(true);
    extractedEdit->setPlaceholderText("Extracted 24-bit values");
    extractedEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    extractedEdit->setMinimumHeight(300);

    // --- Tared values window ---
    taredEdit = new QTextEdit(this);
    taredEdit->setReadOnly(true);
    taredEdit->setPlaceholderText("Tared values");
    taredEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    taredEdit->setMinimumHeight(300);

    // --- Scaling values window ---
    scalingEdit = new QTextEdit(this);
    scalingEdit->setReadOnly(true);
    scalingEdit->setPlaceholderText("Scaling values");
    scalingEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scalingEdit->setMinimumHeight(300);

    // --- Status pane ---
    statusEdit = new QTextEdit(this);
    statusEdit->setReadOnly(true);
    statusEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    statusEdit->setMinimumHeight(300);

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

    tareButton = new QPushButton("Set Tare:", this);
    tareLayout->addWidget(tareButton);

    tareInput = new QLineEdit(this);
    tareInput->setPlaceholderText("Enter taring value");
    tareInput->setMaximumWidth(100);
    tareLayout->addWidget(tareInput);
    tareInput->setText(QString::number(tareValue));



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

    // --- Scaling factor input field ---
    scalingFactorInput = new QLineEdit(this);
    scalingFactorInput->setMaximumWidth(120);
    scalingFactorInput->setText(QString::number(scalingFactor));

    scalingFactorButton = new QPushButton("Apply", this);
    tareLayout->addWidget(new QLabel("Scaling Factor:"));
    tareLayout->addWidget(scalingFactorInput);
    tareLayout->addWidget(scalingFactorButton);

    connect(scalingFactorButton, &QPushButton::clicked, this, [this]() {
        bool ok;
        int value = scalingFactorInput->text().toInt(&ok);
        if (ok && value != 0) {
            scalingFactor = value;
            statusEdit->append(QString("Scaling factor set to %1").arg(scalingFactor));
        } else {
            statusEdit->append("Invalid scaling factor");
        }
    });

    // --- Layouts for each column ---
    QVBoxLayout *rawLayout = new QVBoxLayout();
    rawLayout->addWidget(new QLabel("Raw Data"));
    rawLayout->addWidget(rawEdit);

    QVBoxLayout *extractedLayout = new QVBoxLayout();
    extractedLayout->addWidget(new QLabel("Extracted Values"));
    extractedLayout->addWidget(extractedEdit);

    QVBoxLayout *taredLayout = new QVBoxLayout();
    taredLayout->addWidget(new QLabel("Tared Values"));
    taredLayout->addWidget(taredEdit);

    QVBoxLayout *scalingLayout = new QVBoxLayout();
    scalingLayout->addWidget(new QLabel("Weight in grams"));
    scalingLayout->addWidget(scalingEdit);

    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->addWidget(new QLabel("Status"));
    statusLayout->addWidget(statusEdit);

    // --- Top row with all columns ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addLayout(rawLayout, 1);
    topLayout->addLayout(extractedLayout, 1);
    topLayout->addLayout(taredLayout, 1);
    topLayout->addLayout(scalingLayout, 1);
    topLayout->addLayout(statusLayout, 1);

    // --- Bottom row with controls ---
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(startStopButton);
    bottomLayout->addStretch();
    bottomLayout->addLayout(tareLayout);

    // --- Main layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(topLayout, 1);     // top layout gets extra space
    mainLayout->addLayout(bottomLayout, 0);  // bottom layout stays compact

    QWidget *central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    // --- Resize window and center on screen ---
    resize(1200, 600); // wider to fit new column
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());

    // --- Initialize FTDI ---
    ftdi = ftdi_new();
    if (!ftdi) {
        rawEdit->append("Failed to allocate FTDI context");
        return;
    }

    if (ftdi_usb_open(ftdi, 0x0403, 0x6001) < 0) {
        rawEdit->append(ftdi_get_error_string(ftdi));
        return;
    }

    ftdi_set_baudrate(ftdi, 9600);

    // --- Timer to poll FTDI ---
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::readFtdiData);
    timer->start(50);

    // --- Timer for colored OK messages ---
    // okTimer = new QTimer(this);
    // connect(okTimer, &QTimer::timeout, this, [this]() {
    //     static int colorIndex = 0;
    //     const QStringList colors = {"green", "red", "blue"};
    //     QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    //     statusEdit->append(QString("<span style=\"color:%1;\">[%2] OK</span>")
    //                        .arg(colors[colorIndex])
    //                        .arg(ts));

    //     colorIndex = (colorIndex + 1) % colors.size();
    // });
    // okTimer->start(1000);
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

// #define DEBUG_FTDI

void MainWindow::readFtdiData()
{
    if (!readingEnabled)
        return;

    enum {
        EXPECT_12,
        EXPECT_13,
        EXPECT_14
    };

    static int state = EXPECT_12;
    static uint8_t bytes[3];
    static QString tripletTimestamp;

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
    // printf("Read %d bytes from FTDI\n", n);
    if (n <= 0) return;
#endif

    QByteArray data(reinterpret_cast<char*>(buf), n);
    QList<QByteArray> lines = data.split('\n');

    for (const QByteArray &line : lines) {
        QString strLine = QString::fromUtf8(line).trimmed();

        if (!strLine.startsWith("[2AWA"))
            continue;

        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

        // ---- RAW WINDOW ----
        rawEdit->append(QString("[%1] %2").arg(timestamp, strLine));

        // ---- Extract data byte ----
        int rIndex = strLine.indexOf("[2AR");
        if (rIndex == -1 || rIndex + 5 >= strLine.length()) {
            state = EXPECT_12;
            // statusEdit->append("Could not extract value");
            continue;
        }

        QString byteStr = strLine.mid(rIndex + 5, 2);
        bool ok;
        uint8_t value = byteStr.toUInt(&ok, 16);
        if (!ok) {
            state = EXPECT_12;
            // statusEdit->append("Could not extract value");
            continue;
        }

        // ---- State machine ----
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

            // ---- Complete triplet ----
            uint32_t result =
                (bytes[0] << 16) |
                (bytes[1] << 8)  |
                 bytes[2];

            result *= 1000;
            extractedEdit->append(
                QString("[%1] %2").arg(tripletTimestamp).arg(result)
            );

            int tared = result - tareValue;
            taredEdit->append(
                QString("[%1] %2").arg(tripletTimestamp).arg(tared)
            );

            float grams = (static_cast<float>(tared) / static_cast<float>(scalingFactor));

            scalingEdit->append(
                QString("[%1] %2").arg(tripletTimestamp).arg(grams, 0, 'f', 3)
            );

            state = EXPECT_12;
        }
        else {
            // ---- Broken sequence ----
            state = EXPECT_12;
            // statusEdit->append("Broken sequence, could not extract value");
        }
    }
}