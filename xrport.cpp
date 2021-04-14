#include "xrport.h"
#include "xrdevice.h"

namespace XrProtokol {

Port::Port(Device* device)
    : device(device) //
{
    setBaudRate(Baud115200);
    setDataBits(Data8);
    setFlowControl(NoFlowControl);
    setParity(NoParity);

    connect(device, &Device::open, this, &Port::openSlot);
    connect(device, &Device::close, this, &Port::closeSlot);
    connect(device, &Device::write, this, &Port::writeSlot);

    connect(this, &QSerialPort::readyRead, this, &Port::readSlot, Qt::DirectConnection);
}

void Port::openSlot(int mode) {
    QMutexLocker locker(&m_mutex);
    if(open(static_cast<OpenMode>(mode)))
        device->m_semaphore.release();
}

void Port::closeSlot() {
    QMutexLocker locker(&m_mutex);
    close();
    device->m_semaphore.release();
}

void Port::writeSlot(const QByteArray& data) {
    QMutexLocker locker(&m_mutex);
    counter += write(data);
    qDebug() << __FUNCTION__ << data.toHex('|').toUpper();
}

void Port::readSlot() {
    QMutexLocker locker(&m_mutex);
    m_answerData.append(readAll());
    qDebug() << __FUNCTION__ << m_answerData.toHex('|').toUpper();
    for(int i = 0; i < m_answerData.size() - 3; ++i) {
        const Parcel* const d = reinterpret_cast<const Parcel*>(m_answerData.constData() + i);
        if(d->start == RX && d->len <= m_answerData.size()) {
            QByteArray tmpData = m_answerData.mid(i, d->len);
            counter += tmpData.size();
            if(Device::checkParcel(tmpData))
                (device->*device->callBacks[d->cmd])(tmpData);
            else
                (device->*device->callBacks[SysCmd::CrcError])(tmpData);

            device->m_semaphore.release();
            m_answerData.remove(0, i + d->len);
            i = 0;
        }
        //        if (m_answerData.at(i) == -86 && m_answerData.at(i + 1) == 85) {
        //            if ((i + m_answerData[i + 2]) <= m_answerData.size()) {
        //                m_tmpData = m_answerData.mid(i, m_answerData[i + 2]);
        //                quint8 cmd = *(quint8*)(m_tmpData.constData() + 3);
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
