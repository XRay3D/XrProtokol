#ifndef MYPROTOKOL_H
#define MYPROTOKOL_H

#include <QByteArray>

enum {
    TX = 0xAA55,
    RX = 0x55AA,
    MIN_LEN = 5,
    MIN_LEN_ADDR = 6
};

#pragma pack(push, 1)

struct Parcel {
    //Parcel() = delete;
    uint16_t start;
    uint8_t len;
    uint8_t cmd;
    uint8_t data[1];
};

#pragma pack(pop)

class MyProtokol {
public:
    //////////////////////////////////////////////////
    /// \brief MyProtokol
    ///
    explicit MyProtokol() { }

    //////////////////////////////////////////////////
    /// \brief parcel
    /// \param cmd
    /// \return
    ///
    //    [[nodiscard]] QByteArray parcel(uint8_t cmd)
    //    {
    //        QByteArray data(MIN_LEN, 0);
    //        Parcel* d = reinterpret_cast<Parcel*>(data.data());
    //        *d = { TX, MIN_LEN, static_cast<uint8_t>(cmd), { 0 } };
    //        d->data[0] = calcCrc(data); //crc
    //        return data;
    //    }

    //////////////////////////////////////////////////
    /// \brief parcel
    /// \param cmd
    /// \param value
    /// \return
    template <typename... Ts>
    [[nodiscard]] QByteArray parcel(uint8_t cmd, Ts&&... value)
    {
        constexpr size_t size(sizeof(Ts)...);
        QByteArray data(MIN_LEN + size, 0);
        Parcel* d = reinterpret_cast<Parcel*>(data.data());
        *d = {
            .start = TX,
            .len = static_cast<uint8_t>(data.size()),
            .cmd = cmd,
            .data = { 0 }
        };
        size_t offset {};
        ((*reinterpret_cast<std::decay_t<Ts>*>(d->data + offset) = value, offset + sizeof(Ts)), ...);
        d->data[size] = calcCrc(data); //crc
        return data;
    }

    //////////////////////////////////////////////////
    /// \brief value
    /// \param data
    /// \return
    ///
    template <typename T>
    [[nodiscard]] T value(const QByteArray& data) const
    {
        const Parcel* d = reinterpret_cast<const Parcel*>(data.constData());
        const T val(*reinterpret_cast<const T*>(d->data));
        return val;
    }

    //////////////////////////////////////////////////
    /// \brief pValue
    /// \param data
    /// \return
    ///
    template <typename T>
    [[nodiscard]] T* pValue(const QByteArray& data) const
    {
        const Parcel* d = reinterpret_cast<const Parcel*>(data.constData());
        return reinterpret_cast<const T*>(d->data);
    }

    //////////////////////////////////////////////////
    /// \brief checkParcel
    /// \param data
    /// \return
    ///
    [[nodiscard]] bool checkParcel(const QByteArray& data)
    {
        const Parcel* const d = reinterpret_cast<const Parcel*>(data.constData());
        return data.size() >= MIN_LEN
            && d->start == RX
            && d->len == data.size()
            && d->data[d->len - MIN_LEN] == calcCrc(data);
    }
    [[nodiscard]] bool checkParcel2(const uint8_t* data)
    {
        const Parcel* const d = reinterpret_cast<const Parcel*>(data);
        return d->start == RX
            && d->len >= MIN_LEN
            && d->data[d->len - MIN_LEN] == calcCrc2(data, d->len);
    }

    //////////////////////////////////////////////////
    /// \brief calcCrc
    /// \param data
    /// \return
    ///
    [[nodiscard]] uint8_t calcCrc(const QByteArray& data)
    {
        uint8_t crc8 = 0;
        for (int i = 0, len = data.size() - 1; i < len; ++i) {
            crc8 ^= data[i];
            crc8 = crcArray[crc8];
        }
        return crc8;
    }
    [[nodiscard]] uint8_t calcCrc2(const uint8_t* data, uint8_t len)
    {
        uint8_t crc8 = 0;
        --len;
        for (int i = 0; i < len; ++i) {
            crc8 ^= data[i];
            crc8 = crcArray[crc8];
        }
        return crc8;
    }

private:
    // POLYNOMIAL =0x1D // x^8 + x^4 + x^3 + x^2 + 1
    static constexpr uint8_t crcArray[0x100] {
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB,
        0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76,
        0x87, 0x9A, 0xBD, 0xA0, 0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
        0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1,
        0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40, 0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8,
        0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
        0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B, 0x08, 0x15, 0x32, 0x2F,
        0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2,
        0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50,
        0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A,
        0x6C, 0x71, 0x56, 0x4B, 0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
        0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E,
        0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB, 0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43,
        0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
        0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0, 0xE3, 0xFE, 0xD9, 0xC4
    };
};

#endif // MYPROTOKOL_H
