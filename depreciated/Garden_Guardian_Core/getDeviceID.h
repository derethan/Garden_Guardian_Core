#include "wiring_private.h"
#include <FlashStorage.h>


// Define the address in Flash memory where the device ID will be stored
FlashStorage(deviceID, String);
FlashStorage(my_flash_store, int);


// String generateDeviceID() {

//   // Seed the random number generator with a value based on time
//   randomSeed(millis());

//   // Define the characters to be used in the device ID
//   const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
//   const size_t charsetSize = sizeof(charset) - 1;

//   // Generate a random device ID of length 10
//   String deviceID = "";
//   for (int i = 0; i < 10; i++) {
//     deviceID += charset[random(0, charsetSize)];
//   }

//   return deviceID;
// }

// String getID () {

//   //Read the ID from Memory
//   String storedDeviceID = deviceID.read ();

//   //Check if an ID already exists, if not Generate new ID
//   if (storedDeviceID == 0){
//     storedDeviceID = generateDeviceID ();
//     deviceID.write(storedDeviceID);
//   }

//   return storedDeviceID;
// }