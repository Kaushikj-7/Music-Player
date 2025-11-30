#ifndef BCDC40F5_1B9D_4B95_BA6D_42895A763232
#define BCDC40F5_1B9D_4B95_BA6D_42895A763232

#endif /* BCDC40F5_1B9D_4B95_BA6D_42895A763232 */
#pragma once
#include "wav_decoder.h"
#include <thread>
#include <atomic>

class MockPlayer {
public:
    void play(const WavData& data);
};
