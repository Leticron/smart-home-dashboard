// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE10
#error "Wrong board selection for this example, please select Inkplate 10 in the boards menu."
#endif

#include "HTTPClient.h"          //Include library for HTTPClient
#include "Inkplate.h"            //Include Inkplate library to the sketch
#include "WiFi.h"                //Include library for WiFi
#include "driver/rtc_io.h"       //ESP32 library used for deep sleep and RTC wake up pins
#include "PubSubClient.h"        //mqtt connection
#include "ArduinoJson.h"         //Arduino JSON Library for generating JSON formats for mqtt client

Inkplate display(INKPLATE_3BIT);

#define uS_TO_S_FACTOR 1000000 // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  1200      // How long ESP32 will be in deep sleep (in seconds)

const char *ssid = ""; // Your WiFi SSID
const char *password = ""; // Your WiFi password
const char *mqtt_server = "192.168.1.200";

WiFiClient espClient;
PubSubClient client(espClient);

void connectWifi()
{
    // Connect to the WiFi network.
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print(display.isConnected()); 
}

void displayImage()
{
    // display image on screen
    display.begin();        // Init Inkplate library (you should call this function ONLY ONCE)
    display.clearDisplay(); // Clear frame buffer of display
    display.display();      // Put clear image on display
    // display.setRotation(3);  // Display rotated to portrait mode for testing

    Serial.println("\nWiFi OK! Downloading...");

    if (!display.drawImage("http://192.168.1.71:8080/cover.jpg", 0, 0, false, false))
    {
        Serial.println("Image open error");
    }
    display.display();
}

// reconnect the mqtt connection
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect, sending the client name
        if (client.connect("Inkplate"))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void sendMqttMsg()
{
      // publish voltage and temperature on mqtt
    client.setServer(mqtt_server, 1883);

    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    
    int temperature;
    float voltage;
    char buffer[256];

    StaticJsonDocument<200> inkplate; // Declare static Json Document
    
    temperature = display.readTemperature(); // Read temperature from on-board temperature sensor
    voltage = display.readBattery(); // Read battery voltage (NOTE: Due to ESP32 ADC accuracy, you should calibrate the ADC!)
    
    inkplate["device"] = "Inkplate"; // name the device
    inkplate["temperature"] = temperature; // add inkplate temperature 
    inkplate["voltage"] = voltage; // add inkplate battery voltage

    serializeJson(inkplate, buffer);

    Serial.println(buffer);

    client.publish("home-assistant/inkplate", buffer);
}

void goToSleep()
{
    WiFi.mode(WIFI_OFF);
    rtc_gpio_isolate(GPIO_NUM_12); // Isolate/disable GPIO12 on ESP32 (only to reduce power consumption in sleep)
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // Activate wake-up timer -- wake up after 20s here
    esp_deep_sleep_start();                                        // Put ESP32 into deep sleep. Program stops here.
}

void setup()
{
    Serial.begin(115200);
    connectWifi();
    displayImage();
    sendMqttMsg();
    goToSleep();
}

void loop()
{
    // Nothing...
}
