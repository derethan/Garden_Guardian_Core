#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include "wifiControl.h"

class Controller
{
public:
    void processCommand(String incomingMessage, WifiControl &wifiCon);
};

#endif // CONTROLLER_H
