#ifndef TEMPHUM_DEVICECONFIG_H
#define TEMPHUM_DEVICECONFIG_H

#include "base/deviceConfig.h"

/*****************************************
 * Temperature/Humidity Device Specific Configuration
 *****************************************/

// Forward declaration of the SystemState struct
struct SystemState;

class TempHumDeviceSettingsApplier : public DeviceSettingsApplier 
{
private:
    SystemState& state;
    
public:
    explicit TempHumDeviceSettingsApplier(SystemState& systemState) : state(systemState) {}
    
    void applySettings(const DeviceSettings& settings) override;
    void displaySettings(const DeviceSettings& settings) override;
};

#endif // TEMPHUM_DEVICECONFIG_H
