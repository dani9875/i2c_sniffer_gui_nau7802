#include "ftdireader.h"

FtdiReader::FtdiReader(ftdi_context *ctx, QObject *parent)
    : QObject(parent), ftdi(ctx)
{
}

FtdiReader::~FtdiReader()
{
    stop();
}

void FtdiReader::start()
{
    running = true;

    unsigned char buf[16384];

    while (running) {
        int n = ftdi_read_data(ftdi, buf, sizeof(buf));
        if (n > 0) {
            emit bytesReceived(QByteArray(reinterpret_cast<char*>(buf), n));
        } else if (n < 0) {
            emit error("FTDI read error");
            break;
        }
    }
}

void FtdiReader::stop()
{
    running = false;
}
