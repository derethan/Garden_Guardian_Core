#ifndef BASE_DEVICECONFIG_H
#define BASE_DEVICECONFIG_H

#include "../networkConnections.h"

/*****************************************
 * Base Device Configuration Interface
 *****************************************/

// Abstract interface for device-specific setting application
class DeviceSettingsApplier 
{
public:
    virtual ~DeviceSettingsApplier() = default;
    virtual void applySettings(const DeviceSettings& settings) = 0;
    virtual void displaySettings(const DeviceSettings& settings) = 0;
};

namespace BaseConfig {
    // Base function that handles the generic loading and application logic
    bool loadAndApplyDeviceSettings(NetworkConnections& network, DeviceSettingsApplier* applier);
}

#endif // BASE_DEVICECONFIG_H
