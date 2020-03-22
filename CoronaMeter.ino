// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

#include <WiFi.h>
#include <ArduinoJson.h>

struct ClientData {
  int localNewCases = 0;
  int localTotalCases = 0;
  int localInHospital = 0;
  int localDeaths = 0;
  int localNewDeaths = 0;
} clientData;

const unsigned long HTTP_TIMEOUT = 10000;  

const char * ssid = "<WIFI SSID>";
const char * password = "<WIFI PASSWORD>";
const char * api_server = "healthpromo.gov.lk";
const char * api_endpoint = "/api/get-current-statistical";

WiFiClient client;

// Initialize the OLED display using Arduino Wire:
SSD1306Wire display(0x3c, 32, 33);   // ADDRESS, SDA - is connected to D32 Pin, SCL - is connected to D33 Pin

#define DEMO_DURATION 3000
#define DATA_FETCH_DURATION 1000 * 60 * 60
typedef void (*Screen)(void);

int demoMode = 0;
int counter = 1;
long timeSinceLastFetch = 0;

bool connect(const char* api_server) {
  Serial.print("Connect to ");
  Serial.println(api_server);

  bool ok = client.connect(api_server, 80);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

void disconnect() {
  client.stop();
}

void connectToApi() {
  if (connect(api_server)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    if (sendRequest(api_server, api_endpoint) && skipResponseHeaders()) {
      if(readReponseContent(&clientData)) {
        printclientData(&clientData);
      }
    }
  }
  disconnect();
}

bool sendRequest(const char * server, const char * endpoint) {
  client.print("GET ");
  client.print(endpoint); 
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(server);
  client.println("Connection: close");
  client.println();  
}

bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }
  return ok;
}

bool readReponseContent(struct ClientData* clientData) {
  const size_t capacity = JSON_ARRAY_SIZE(18) + JSON_OBJECT_SIZE(3) + 18*JSON_OBJECT_SIZE(7) + 18*JSON_OBJECT_SIZE(12) + JSON_OBJECT_SIZE(13) + 8120;
  DynamicJsonBuffer jsonBuffer(capacity);
  
  JsonObject& root = jsonBuffer.parseObject(client);
  
  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }
  
  bool success = root["success"]; // true
  const char* message = root["message"]; // "Success"
  
  JsonObject& data = root["data"];
  const char* data_update_date_time = data["update_date_time"]; // "2020-03-21 18:57:17"
  int data_local_new_cases = data["local_new_cases"]; // 5
  int data_local_total_cases = data["local_total_cases"]; // 77
  int data_local_total_number_of_individuals_in_hospitals = data["local_total_number_of_individuals_in_hospitals"]; // 245
  int data_local_deaths = data["local_deaths"]; // 0
  int data_local_new_deaths = data["local_new_deaths"]; // 0
  int data_local_recovered = data["local_recovered"]; // 1
  int data_global_new_cases = data["global_new_cases"]; // 30665
  long data_global_total_cases = data["global_total_cases"]; // 283256
  int data_global_deaths = data["global_deaths"]; // 11829
  int data_global_new_deaths = data["global_new_deaths"]; // 1061
  long data_global_recovered = data["global_recovered"]; // 91573  

  clientData->localNewCases = data_local_new_cases;
  clientData->localTotalCases = data_local_total_cases;
  clientData->localInHospital = data_local_total_number_of_individuals_in_hospitals;
  clientData->localDeaths = data_local_deaths;
  clientData->localNewDeaths = data_local_new_deaths;

  return true;
}

void printclientData(const struct ClientData* clientData) {
  Serial.print("New Cases = ");
  Serial.println(clientData->localNewCases);
  Serial.print("Total Cases = ");
  Serial.println(clientData->localTotalCases);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  Serial.println("Going to connect Wifi...");

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("Waiting for Wifi...");
    delay(500);
  }

  Serial.print("Connected to Wifi. IP Address : ");
  Serial.println(WiFi.localIP());


  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  connectToApi();
  timeSinceLastFetch = millis();

}

void drawCoronaTitle() {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 0, "Corona");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 24, "Stats");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 40, "Sri Lanka");
}

void drawLocalNewCases() {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 0, "New Cases");  
    display.setFont(ArialMT_Plain_24);
    String myString = String(clientData.localNewCases);
    display.drawString(64, 28, myString);    
}

void drawLocalTotalCases() {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 0, "Total Cases");  
    display.setFont(ArialMT_Plain_24);
    String myString = String(clientData.localTotalCases);
    display.drawString(64, 28, myString);    
}

void drawInHospital() {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 0, "In Hospital");  
    display.setFont(ArialMT_Plain_24);
    String myString = String(clientData.localInHospital);
    display.drawString(64, 28, myString);    
}

void drawLocalTotalDeaths() {
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 0, "Deaths");  
    display.setFont(ArialMT_Plain_24);
    String myString = String(clientData.localDeaths);
    display.drawString(64, 28, myString);    
}

Screen demos[] = {drawCoronaTitle, drawLocalNewCases, drawLocalTotalCases, drawInHospital, drawLocalTotalDeaths};
int demoLength = (sizeof(demos) / sizeof(Screen));
long timeSinceLastModeSwitch = 0;

void loop() {
  // clear the display
  display.clear();
  // draw the current demo method
  demos[demoMode]();

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(10, 128, String(millis()));
  // write the buffer to the display
  display.display();

  if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
    demoMode = (demoMode + 1)  % demoLength;
    timeSinceLastModeSwitch = millis();
  }
  counter++;
  delay(10);
  if (millis() - timeSinceLastFetch > DATA_FETCH_DURATION) {
    connectToApi();
    timeSinceLastFetch = millis();
  }
}
