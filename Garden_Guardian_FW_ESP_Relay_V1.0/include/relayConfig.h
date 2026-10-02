#pragma once
#include <Arduino.h>

#define RELAY_COUNT 4

enum class RelayMode : uint8_t
{
  SCHEDULE = 0,
  TEMPERATURE = 1,
  TIMED = 2,
};

enum class TempSource : uint8_t
{
  AIR = 0,
  DWC = 1,
  NFT = 2,
};

struct RelayConfig
{
  RelayMode mode = RelayMode::SCHEDULE;
  TempSource tempSource = TempSource::AIR;
  uint8_t onHour = 0;
  uint8_t offHour = 18;
  uint16_t onMinutes = 30;  // Timed mode: minutes ON
  uint16_t offMinutes = 30; // Timed mode: minutes OFF
};

// Defaults reproduce the original hard-wired behaviour
inline RelayConfig defaultRelayConfig(int index)
{
  RelayConfig c;
  switch (index)
  {
  case 0:
    c.mode = RelayMode::SCHEDULE;
    break;
  case 1:
    c.mode = RelayMode::TEMPERATURE;
    c.tempSource = TempSource::AIR;
    break;
  case 2:
    c.mode = RelayMode::TEMPERATURE;
    c.tempSource = TempSource::DWC;
    break;
  default:
    c.mode = RelayMode::TEMPERATURE;
    c.tempSource = TempSource::NFT;
    break;
  }
  return c;
}
