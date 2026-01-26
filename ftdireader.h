#pragma once

#include <QObject>
#include <QByteArray>
#include <atomic>
#include <ftdi.h>

class FtdiReader : public QObject
{
    Q_OBJECT
public:
    explicit FtdiReader(ftdi_context *ctx, QObject *parent = nullptr);
    ~FtdiReader();

public slots:
    void start();
    void stop();

signals:
    void bytesReceived(QByteArray data);
    void error(QString msg);

private:
    ftdi_context *ftdi;
    std::atomic<bool> running{false};
};
