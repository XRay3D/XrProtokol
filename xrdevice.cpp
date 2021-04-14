#include "xrdevice.h"

#include <span>

namespace XrProtokol {

Device::Device(QObject* parent)
    : QObject(parent)
    , m_port{new Port(this)}
    , callBacks(0x100, &Device::ioRxWrongCommand) //
{
    callBacks[SysCmd::Ping] = &Device::ioRxPing;
    callBacks[SysCmd::BufferOverflow] = &Device::ioRxBufferOverflow;
    callBacks[SysCmd::WrongCommand] = &Device::ioRxWrongCommand;
    callBacks[SysCmd::CrcError] = &Device::ioRxCrcError;

    m_port->moveToThread(&m_portThread);
    connect(&m_portThread, &QThread::finished, m_port, &QObject::deleteLater);
    m_portThread.start(QThread::NormalPriority);
}

Device::~Device() {
    m_portThread.quit();
    m_portThread.wait();
}

bool Device::ping(const QString& portName, int baud, [[maybe_unused]] int addr) {
    QMutexLocker locker(&m_mutex);
    m_connected = false;
    reset();
    do {
        emit close();
        if(!m_semaphore.tryAcquire(1, 1000))
            break;

        if(!portName.isEmpty())
            m_port->setPortName(portName);

        if(baud)
            m_port->setBaudRate(baud);

        emit open(QIODevice::ReadWrite);
        if(!m_semaphore.tryAcquire(1, 1000))
            break;

        emit write(parcel(static_cast<quint8>(SysCmd::Ping)));
        if(!m_semaphore.tryAcquire(1, 100)) {
            emit close();
            break;
        }

        m_connected = true;
    } while(0);
    return m_connected;
}

bool Device::checkParcel(const QByteArray& data) {
    const Parcel* const d = reinterpret_cast<const Parcel*>(data.constData());
    return data.size() >= MIN_LEN
        && d->start == RX
        && d->len == data.size()
        && d->data[d->len - MIN_LEN] == calcCrc(data);
}

bool Device::checkParcel(const uint8_t* data) {
    const Parcel* const d = reinterpret_cast<const Parcel*>(data);
    return d->start == RX
        && d->len >= MIN_LEN
        && d->data[d->len - MIN_LEN] == calcCrc(data, d->len);
}

uint8_t Device::calcCrc(const QByteArray& data) {
    uint8_t crc8 = 0;
    for(char byte : std::span(data.data(), data.size() - 1)) {
        crc8 ^= byte;
        crc8 = crcArray[crc8];
    }
    return crc8;
}

uint8_t Device::calcCrc(const uint8_t* data, uint8_t len) {
    uint8_t crc8 = 0;
    --len;
    for(int i = 0; i < len; ++i) {
        crc8 ^= data[i];
        crc8 = crcArray[crc8];
    }
    return crc8;
}

Port* Device::port() const { return m_port; }

void Device::ioRxPing(const QByteArray& data) {
    qDebug() << __FUNCTION__ << data.toHex().toUpper();
}

void Device::ioRxBufferOverflow(const QByteArray& data) {
    qDebug() << __FUNCTION__ << data.toHex().toUpper();
}

void Device::ioRxWrongCommand(const QByteArray& data) {
    qDebug() << __FUNCTION__ << data.toHex().toUpper();
}

void Device::ioRxCrcError(const QByteArray& data) {
    qDebug() << __FUNCTION__ << data.toHex().toUpper();
}

void Device::ioRxText(const QByteArray& data) {
    qDebug() << __FUNCTION__ << data << data.toHex().toUpper();
}

}
