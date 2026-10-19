#pragma once
#include <stdint.h>
namespace config {
constexpr int RX_PIN = 1; // White wire on C126-ECHO
constexpr int TX_PIN = 2; // Yellow wire on C126-ECHO
constexpr uint8_t LEFT_ROTATION = 0;  // 0,1,2,3 = 0,90,180,270 degrees
constexpr uint8_t RIGHT_ROTATION = 0;
constexpr bool SWAP_EYES = false;
constexpr uint8_t BRIGHTNESS_LIMIT = 35; // U218: percent, keep <=50
constexpr int ENCODER_DIRECTION = 1; // Change to -1 to reverse
constexpr int ENCODER_COUNTS_PER_COLOR = 1; // If one click changes 2/4 colors, set 2/4
constexpr uint32_t LONG_PRESS_MS = 1500;
constexpr uint32_t FRAME_MS = 50;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr int SAMPLES = 320;
constexpr float NOISE_RATIO = 2.8f;
constexpr float MIN_RMS = 250.0f; // Tune from Serial monitor in the actual costume
static_assert(ENCODER_COUNTS_PER_COLOR>0,"Encoder counts per color must be positive");
static_assert(LEFT_ROTATION<4 && RIGHT_ROTATION<4,"Rotation must be 0..3");
static_assert(BRIGHTNESS_LIMIT<=50,"Keep the U218 brightness limit at or below 50%");
}
