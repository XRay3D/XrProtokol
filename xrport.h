#pragma once

#include <QMutex>
#include <QSerialPort>

namespace XrProtokol {

class Device;
struct Parcel;

class Port : public QSerialPort {
    Q_OBJECT

public:
    Port(Device* device);

private:
    void openSlot(int mode);
    void closeSlot();
    void writeSlot(const Parcel& data);

    void readSlot();
    QByteArray m_answerData;
    QMutex m_mutex;
    qint64 counter = 0;
    Device* device;
};
}
