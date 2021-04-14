#pragma once

#include <QMutex>
#include <QSerialPort>

namespace XrProtokol {

class Device;

class Port : public QSerialPort {
    Q_OBJECT

public:
    Port(Device* device);
    void openSlot(int mode);
    void closeSlot();
    void writeSlot(const QByteArray& data);
    Device* device;

private:
    void readSlot();
    QByteArray m_answerData;
    QMutex m_mutex;
    qint64 counter = 0;
};
}
