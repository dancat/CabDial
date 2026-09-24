#include <Arduino.h>
#include "AppController.h"

void initializeApplication();
void updateApplication();

void setup()
{
    initializeApplication();
}

void loop()
{
    updateApplication();
}
