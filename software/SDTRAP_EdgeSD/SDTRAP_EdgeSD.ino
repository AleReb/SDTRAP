#include <Arduino.h>

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("SDTRAP-EdgeSD");
    Serial.println("Firmware scaffold initialized.");
}

void loop()
{
    // Future:
    // - camera SD activity monitoring
    // - SD ownership state machine
    // - image discovery
    // - edge inference
    // - Wi-Fi / LTE transmission
}
