#include "I2SManager.h"

bool I2SManager::begin()
{
    m_i2s.setPins(
        26,  // BCK
        25,  // LCK / WS
        22   // DIN / DOUT
    );

    return m_i2s.begin(
        I2S_MODE_STD,
        44100,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO
    );
}

size_t I2SManager::write(const uint8_t* data, size_t length)
{
    if (data == nullptr || length == 0)
    {
        return 0;
    }

    return m_i2s.write(data, length);
}