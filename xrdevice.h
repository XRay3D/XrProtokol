#pragma once

#include "xrport.h"

#include <QByteArray>
#include <QDebug>
#include <QMutex>
#include <QSemaphore>
#include <QSerialPort>
#include <QThread>
#include <commoninterfaces.h>

namespace XrProtokol {

enum {
    TX = 0xAA55,
    RX = 0x55AA,
    MIN_LEN = 5,
    MIN_LEN_ADDR = 6,
    DbgMsg = 0,
};

struct SysCmd {
    enum : uint8_t {
        Ping = /*           */ 0x00,
        Reserve1 = /*       */ 0xFA,
        Reserve2 = /*       */ 0xFB,
        BufferOverflow = /* */ 0xFC,
        CrcError = /*       */ 0xFD,
        Text = /*           */ 0xFE,
        WrongCommand = /*   */ 0xFF,
    };
};

enum class Type : uint8_t {
    AMK_Tester,
    AdcSwitch = 100,
    UPN_V2 = 101,
};

class Device;
class Port;

[[nodiscard]] uint8_t calcCrc(const QByteArray& data);
[[nodiscard]] uint8_t calcCrc(const uint8_t* data, uint8_t len);

#pragma pack(push, 1)

struct Parcel {
    uint16_t start {};
    uint8_t size {};
    uint8_t cmd {};
    uint8_t data[0x100 - MIN_LEN] { 0 };

    Parcel()
    {
    }

    Parcel(const QByteArray& ba)
    {
        *this = *reinterpret_cast<const Parcel*>(ba.data());
    }

    template <class... Ts>
    Parcel(uint8_t cmd, Ts... value)
        : cmd(cmd)
    {
        constexpr uint8_t size_ { (sizeof(Ts) + ... + 0) };
        start = TX,
        size = static_cast<uint8_t>(size_ + MIN_LEN);
        size_t offset {};
        ((*reinterpret_cast<std::decay_t<Ts>*>(data + offset) = value, offset += sizeof(Ts)), ...);
        data[size_] = calcCrc(reinterpret_cast<const uint8_t*>(this), size); //crc
    }

    QByteArray toHex(char delim = {}) const { return QByteArray(reinterpret_cast<const char*>(this), size).toHex(delim).toUpper(); }
    template <typename T>
    void toValue(T& value) const { value = *reinterpret_cast<const T*>(data); }
    template <typename T>
    T toValue() const { return *reinterpret_cast<const T*>(data); }
};

using CallBack = void (Device::*)(const Parcel&);

#pragma pack(pop)

class Device : public QObject, public CommonInterfaces {
    Q_OBJECT
    friend Port;

signals:
    void open(int mode) override;
    void writeParcel(const XrProtokol::Parcel& data);
    void close_();
    void message(const QString& msg, int timeout = 0);

public:
    //////////////////////////////////////////////////
    /// \brief MyProtokol
    ///
    explicit Device(QObject* parent = nullptr);
    ~Device();

    bool ping(const QString& portName = {}, int baud = QSerialPort::Baud115200, int addr = {}) override;
    void close() override;
    void reset() override;
    virtual Type type() const = 0;

    [[nodiscard]] static bool checkParcel(const QByteArray& data);
    [[nodiscard]] static bool checkParcel(const uint8_t* data);

    //////////////////////////////////////////////////

    Port* port() const;

protected:
    void ioRxDefault(const Parcel& data);

    void ioRxPing(const Parcel& data);
    void ioRxBufferOverflow(const Parcel& data);
    void ioRxWrongCommand(const Parcel& data);
    void ioRxCrcError(const Parcel& data);
    void ioRxText(const Parcel& data);

    template <auto Cmd, class Derived>
    void registerCallback(void (Derived::*Func)(const Parcel&)) requires std::is_base_of_v<Device, Derived>
    {
        static_assert(SysCmd::Ping != Cmd, "SysCmd::Ping");
        static_assert(SysCmd::Reserve1 != Cmd, "SysCmd::Reserve1");
        static_assert(SysCmd::Reserve2 != Cmd, "SysCmd::Reserve2");
        static_assert(SysCmd::BufferOverflow != Cmd, "SysCmd::BufferOverflow");
        static_assert(SysCmd::CrcError != Cmd, "SysCmd::CrcError");
        static_assert(SysCmd::Text != Cmd, "SysCmd::Text");
        static_assert(SysCmd::WrongCommand != Cmd, "SysCmd::WrongCommand");
        callBacks[Cmd] = reinterpret_cast<CallBack>(Func);
    }

    QSemaphore semaphore_;
    Parcel lastParcel;
    QMutex mutex;

private:
    Port* port_;
    QThread portThread;

    std::array<CallBack, 0x100> callBacks;
};

}
Q_DECLARE_METATYPE(XrProtokol::Parcel)
