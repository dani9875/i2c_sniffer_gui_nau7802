#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // --- Main text edit for serial data ---
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);

    // --- Status pane ---
    statusEdit = new QTextEdit(this);
    statusEdit->setReadOnly(true);

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

    // --- Status layout ---
    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->addWidget(statusEdit);
    statusLayout->addLayout(tareLayout);

    // --- Main layout ---
    QWidget *central = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(central);
    layout->addWidget(textEdit, 3);         // Serial pane
    layout->addLayout(statusLayout, 1);     // Status pane with tare
    setCentralWidget(central);

    // --- Initialize FTDI ---
    ftdi = ftdi_new();
    if (!ftdi) {
        textEdit->append("Failed to allocate FTDI context");
        return;
    }

    if (ftdi_usb_open(ftdi, 0x0403, 0x6001) < 0) {
        textEdit->append(ftdi_get_error_string(ftdi));
        return;
    }

    ftdi_set_baudrate(ftdi, 9600);

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
    okTimer->start(1000); // every 1 second

    // Initialize tare value
    tareValue = 0.0;
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

void MainWindow::readFtdiData()
{
    static int byteCount = 0;
    static uint8_t bytes[3]; // store the 3 bytes from NAU7802

    unsigned char buf[256];
    int n = ftdi_read_data(ftdi, buf, sizeof(buf));
    if (n <= 0) return;

    QByteArray data(reinterpret_cast<char*>(buf), n);

    QList<QByteArray> lines = data.split('\n');
    for (const QByteArray &line : lines) {
        QString strLine = QString::fromUtf8(line).trimmed();
        if (!strLine.startsWith("[2AW"))
            continue;

        // Prepend timestamp for main serial window
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        textEdit->append(QString("[%1] %2").arg(timestamp, strLine));

        // Extract the data byte from the line
        int rIndex = strLine.indexOf("[2AR");
        if (rIndex == -1 || rIndex + 5 >= strLine.length())
            continue;

        QString byteStr = strLine.mid(rIndex + 4, 2); // get "YY"
        bool ok;
        uint8_t value = byteStr.toUInt(&ok, 16);
        if (!ok) continue;

        // Store in bytes array
        if (byteCount < 3) {
            bytes[byteCount++] = value;
        }

        // Once we have 3 bytes, convert to 24-bit value
        if (byteCount == 3) {
            int32_t result = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];

            // Apply tare
            double adjusted = result - tareValue;

            // Prepend timestamp for status window
            QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            statusEdit->append(QString("[%1] %2").arg(ts).arg(adjusted));

            byteCount = 0; // reset for next value
        }
    }
}
