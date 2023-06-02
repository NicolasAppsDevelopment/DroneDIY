#include "WiFi.h"
#include "AsyncUDP.h"
#include "string"

const char* ssid = "Dronator2000";
const char* password = "12345678";

AsyncUDP udp;

#define PORT 1234
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup()
{
    Serial.begin(115200);
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    if(udp.listen(PORT)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(local_ip);
        Serial.print("UDP Listening on port: ");
        Serial.println(PORT);
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("Data received: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
        });
    }
}

int i = 0;
void loop()
{
    delay(100);
    //Send broadcast
    
    const String text = "ping " + String(i);

    const int length = text.length();
  
    // declaring character array (+1 for null terminator)
    char* char_array = new char[length + 1];
  
    // copying the contents of the
    // string to char array
    strcpy(char_array, text.c_str());


    udp.broadcast(char_array);
    i++;
}
