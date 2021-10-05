#include "xrport.h"
#include "xrdevice.h"

namespace XrProtokol {

int idParcel = qRegisterMetaType<Parcel>("Parcel");

Port::Port(Device* device)
    : device(device) //
{
    setBaudRate(Baud115200);
    setDataBits(Data8);
    setFlowControl(NoFlowControl);
    setParity(NoParity);

    connect(device, &Device::open, this, &Port::openSlot, Qt::QueuedConnection);
    connect(device, &Device::close_, this, &Port::closeSlot, Qt::QueuedConnection);
    connect(device, &Device::writeParcel, this, &Port::writeSlot, Qt::QueuedConnection);

    connect(this, &QSerialPort::readyRead, this, &Port::readSlot, Qt::DirectConnection);
}

void Port::openSlot(int mode)
{
    QMutexLocker locker(&m_mutex);
    if (open(static_cast<OpenMode>(mode)))
        device->semaphore_.release();
}

void Port::closeSlot()
{
    QMutexLocker locker(&m_mutex);
    close();
    device->semaphore_.release();
}

void Port::writeSlot(const Parcel& data)
{
    QMutexLocker locker(&m_mutex);
    counter += write(reinterpret_cast<const char*>(&data), data.size);
    if constexpr (DbgMsg)
        qDebug() << __FUNCTION__ << data.toHex('|').toUpper();
}

void Port::readSlot()
{
    QMutexLocker locker(&m_mutex);
    m_answerData.append(readAll());
    if constexpr (DbgMsg)
        qDebug() << __FUNCTION__ << m_answerData.toHex('|').toUpper();
    for (int i = 0; i < m_answerData.size() - 3; ++i) {
        const Parcel* const d = reinterpret_cast<const Parcel*>(m_answerData.constData() + i);
        if (d->start == RX && d->size <= m_answerData.size()) {
            QByteArray tmpData = m_answerData.mid(i, d->size);
            counter += tmpData.size();
            if (Device::checkParcel(tmpData))
                (device->*device->callBacks[d->cmd])(tmpData);
            else
                (device->*device->callBacks[SysCmd::CrcError])(tmpData);

            device->semaphore_.release();
            m_answerData.remove(0, i + d->size);
            i = 0;
        }
        //        if (m_answerData.at(i) == -86 && m_answerData.at(i + 1) == 85) {
        //            if ((i + m_answerData[i + 2]) <= m_answerData.size()) {
        //                m_tmpData = m_answerData.mid(i, m_answerData[i + 2]);
        //                uint8_t cmd = *(uint8_t*)(m_tmpData.constData() + 3);
        //                if (checkParcel(m_tmpData))
        //                    (m_t->*m_f[cmd])(m_tmpData);
        //                else
        //                    (m_t->*m_f[CRC_ERROR])(m_tmpData);
        //                m_answerData.remove(0, i + m_answerData[i + 2]);
        //                i = -1;
        //            }
        //        }
    }
}
}
