#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "Your Network Identifier Goes Here";
const char* password = "Your Network Password Goes Here";
const char* host = "Pick a name for your device here";

ESP8266WebServer server(80);

// ----------------------------------------------------------------
// --------------------- Models & Helpers -------------------------
// ----------------------------------------------------------------
typedef struct
{
    // user friendly name for display in control app
    String name;
    // the pin number of this device
    int pin;
    // device identifier for issuing commands
    String identifier;
    // device description, what is this device for, what does it do?
    String description;
    // the current status of the device. on/off/etc
    String status;
    // the type of device, currently only option is discreet.
    String type;
} Device;


// convenience function for converting Device structs to json objects to provide
// a list of devices that can be controlled on this server.
String deviceToJSON(Device device) {
    String json = "{";
    json += "\"name\":\"" + device.name + "\",";
    json += "\"identifier\":\"" + device.identifier + "\",";
    json += "\"description\":\"" + device.description + "\",";
    json += "\"status\":\"" + device.status + "\",";
    json += "\"type\":\"" + device.type + "\"}";
    return json;
}

// ----------------------------------------------------------------
// ------------------ Setup & Configuration -----------------------
// ----------------------------------------------------------------
void configurePins() {
    int pins[8] = {D1,D2,D3,D4,D5,D6,D7,D8};
    for (int i = 0; i < 8; i++) {
      
        // set pin as output
        pinMode(pins[i], OUTPUT);
        // set pin low to begin
        digitalWrite(pins[i], 0);
    }
}

void configureSerial() {
    Serial.begin(115200);
}

void configureWiFi() {
    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.printf("Default hostname: %s\n", WiFi.hostname().c_str());
    WiFi.hostname(host);
    Serial.printf("New hostname: %s\n", WiFi.hostname().c_str());
    Serial.print("Connecting");
  
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("MDNS responder started");
        Serial.print("Available via http://");
        Serial.print(host);
        Serial.println(".local");
    }
}

void configureServer() {
    server.on("/", handleRoot);
    server.on("/device", handleDevice);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

// ----------------------------------------------------------------
// --------------------- Request Handling -------------------------
// ----------------------------------------------------------------

void handleRoot() {
    // generic response for requests to /
    server.send(200, "text/plain", "hello from esp8266!");
}

void handleDevice() {

    // order of name/pin/identifier/description/status/type
    Device deviceList[8] = {
      {"Relay 1",D1,"d1","Kitchen Lights",digitalRead(D1) == HIGH ? "1" : "0","discreet"},
      {"Relay 2",D2,"d2","Office Lights",digitalRead(D2) == HIGH ? "1" : "0","discreet"},
      {"Relay 3",D3,"d3","Unassigned 3",digitalRead(D3) == HIGH ? "1" : "0","discreet"},
      {"Relay 4",D4,"d4","Unassigned 4",digitalRead(D4) == HIGH ? "1" : "0","discreet"},
      {"Relay 5",D5,"d5","Unassigned 5",digitalRead(D5) == HIGH ? "1" : "0","discreet"},
      {"Relay 6",D6,"d6","Unassigned 6",digitalRead(D6) == HIGH ? "1" : "0","discreet"},
      {"Relay 7",D7,"d7","Unassigned 7",digitalRead(D7) == HIGH ? "1" : "0","discreet"},
      {"Relay 8",D8,"d8","Unassigned 8",digitalRead(D8) == HIGH ? "1" : "0","discreet"}
    };

    // check if this is simply a request for the device list (no query parameters)
    if (server.args() == 0) {
        // start our json string response to return
        String message = "[";

        // iterate the array of devices and convert to json for our server response
        // this provides a list of devices that can be controlled via the server api
        for (int i = 0; i < 8; i++) {
            // append this device json string to our message json string
            message += deviceToJSON(deviceList[i]);

            // if this is not the last device, add a comma, otherwise close our json string
            message += (i < 7) ? "," : "]";
        }
    
        server.send(200, "application/json", message);
        return;
    }
  
    String message = "";

    // we already checked that there are more than 0 args available
    // so grab the first key/value pair from the arguments
    String identifier = server.argName(0);
    String value = server.arg(0);

    // define the value we will be setting the pin with based on whether the
    // value received for this request is a '0' or not
    int val = value == "0" ? LOW : HIGH;
    bool found = false;

    for (int i=0; i<8; i++) {
        // look for a device in our array with a matching identifier as received in the request
        if (identifier == deviceList[i].identifier) {
            // generate a message to send back in our server response
            message += "Device: '" + identifier + "' set: '" + val + "' on pin: " + deviceList[i].pin + ".\n";
            // set the pin for the device matching the requested identifier either high or low
            digitalWrite(deviceList[i].pin, val);
            // mark found so we don't generate an error response message
            found = true;
            break;
        }
    }
    
    if (found == false) {
        // no device matching the identifier was found, generate an appropriate response
        message += "Device '" + identifier + "' not found.\n"; 
    }

    // send a server response
    server.send(200, "text/plain", message);
}

void handleNotFound(){
  
    // generate 404 response
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}







void setup(void){
    configurePins();
    configureSerial();
    configureWiFi(); 
    configureServer();
}

void loop(void){
    server.handleClient();
}
