/**
 * @file deviceConfig.cpp
 * @brief Base device configuration loading and application functionality
 * 
 * Provides the common interface for loading device settings from storage
 * and applying them using device-specific applier implementations.
 */

#include "../../include/base/deviceConfig.h"
#include "../../include/base/sysLogs.h"

/**
 * @namespace BaseConfig
 * @brief Base device configuration functionality namespace
 */
namespace BaseConfig {

/**
 * @brief Load device settings from storage and apply them using a device-specific applier
 * @param network Reference to the network connections object for loading settings
 * @param applier Pointer to device-specific settings applier implementation
 * @return true if settings were loaded and applied successfully, false if using defaults
 */
bool loadAndApplyDeviceSettings(NetworkConnections& network, DeviceSettingsApplier* applier) 
{
    SysLogs::logInfo("SYSTEM", "Loading device settings...");
    
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
        SysLogs::logInfo("SYSTEM", "Using default device settings");
        return false;
    }
}

} // namespace BaseConfig
