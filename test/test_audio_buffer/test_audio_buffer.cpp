#include <Arduino.h>
#include <unity.h>

#include "AudioBuffer.h"

void test_buffer_is_empty_after_begin()
{
    AudioBuffer buffer;

    TEST_ASSERT_TRUE(buffer.begin(8));

    TEST_ASSERT_EQUAL_UINT32(0, buffer.available());
    TEST_ASSERT_EQUAL_UINT32(7, buffer.freeSpace());
}

void test_buffer_write()
{
    AudioBuffer buffer;

    TEST_ASSERT_TRUE(buffer.begin(8));

    uint8_t data[] = {10, 20, 30};

    size_t written = buffer.write(data, 3);

    TEST_ASSERT_EQUAL_UINT32(3, written);
    TEST_ASSERT_EQUAL_UINT32(3, buffer.available());
    TEST_ASSERT_EQUAL_UINT32(4, buffer.freeSpace());
}

void setup()
{
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_buffer_is_empty_after_begin);
    RUN_TEST(test_buffer_write);

    UNITY_END();
}

void loop()
{
}