#include "../../include/base/deviceConfig.h"

/*****************************************
 * Base Device Configuration Functions
 *****************************************/

namespace BaseConfig {

bool loadAndApplyDeviceSettings(NetworkConnections& network, DeviceSettingsApplier* applier) 
{
    Serial.println("[SYSTEM] Loading device settings...");
    
    DeviceSettings settings = network.loadDeviceSettings();
    
    if (settings.valid) 
    {
        // Apply loaded settings using the device-specific applier
        applier->applySettings(settings);
        
        // Display loaded settings using the device-specific applier  
        applier->displaySettings(settings);
        
        return true;
    }
    else 
    {
        Serial.println("[SYSTEM] Using default device settings");
        return false;
    }
}

} // namespace BaseConfig
