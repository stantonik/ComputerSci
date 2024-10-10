#include <WiFi.h>  
#include <WebSocketsServer.h>

const char* ssid = "PoleDeVinci_IFT";
const char* password = "*c.r4UV@VfPn_0";

WebSocketsServer webSocket = WebSocketsServer(80);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) 
{
        switch(type) 
        {
                case WStype_DISCONNECTED:
                        Serial.printf("Client disconnected: %u\n", num);
                        break;
                case WStype_CONNECTED:
                        Serial.printf("Client connected: %u\n", num);
                        break;
                case WStype_TEXT:
                        Serial.printf("Message from client: %s\n", payload);
                        webSocket.sendTXT(num, "Hello from server");
                        break;
        }
}

void setup() 
{
        Serial.begin(115200);  
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) 
        {  
                delay(1000);  
                Serial.println("Connecting to WiFi...");  
        }  
        Serial.println("Connected to WiFi");  
        Serial.print("IP address: ");  
        Serial.println(WiFi.localIP());  
        webSocket.begin();  
        webSocket.onEvent(webSocketEvent);  
}  

void loop() 
{  
        webSocket.loop();
}
