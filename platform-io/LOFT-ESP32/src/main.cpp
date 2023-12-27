#include <Arduino.h>
// #include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GPS.h>
#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

SocketIOclient socketIO;
#define USE_SERIAL Serial
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            USE_SERIAL.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
        {
            char * sptr = NULL;
            int id = strtol((char *)payload, &sptr, 10);
            USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
            if(id) {
                payload = (uint8_t *)sptr;
            }
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload, length);
            if(error) {
                USE_SERIAL.print(F("deserializeJson() failed: "));
                USE_SERIAL.println(error.c_str());
                return;
            }

            String eventName = doc[0];
            USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

            // Message Includes a ID for a ACK (callback)
            if(id) {
                // creat JSON message for Socket.IO (ack)
                DynamicJsonDocument docOut(1024);
                JsonArray array = docOut.to<JsonArray>();

                // add payload (parameters) for the ack (callback function)
                JsonObject param1 = array.createNestedObject();
                param1["now"] = millis();

                // JSON to String (serializion)
                String output;
                output += id;
                serializeJson(docOut, output);

                // Send event
                socketIO.send(sIOtype_ACK, output);
            }
        }
            break;
        case sIOtype_ACK:
            USE_SERIAL.printf("[IOc] get ack: %u\n", length);
            break;
        case sIOtype_ERROR:
            USE_SERIAL.printf("[IOc] get error: %u\n", length);
            break;
        case sIOtype_BINARY_EVENT:
            USE_SERIAL.printf("[IOc] get binary: %u\n", length);
            break;
        case sIOtype_BINARY_ACK:
            USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
            break;
    }
}

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

uint32_t timer = millis();
int timerInterval = 250;

// Replace with your network credentials
const char *ssid = "SSID";
const char *password = "pass";
// const char *ssid = "EvoX";
// const char *password = "milesdavis251";

const char* hostname = "loft.deno.dev";

void setup()
{
  // while (!Serial);  // uncomment to have the sketch wait until Serial is ready

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic parsing test!");

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);

  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);

  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  // Enable debug prints and LED indication if websockets connection fails


  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  IPAddress resolvedIP;
  
  if (!WiFi.hostByName(hostname, resolvedIP)) {
    Serial.println("DNS lookup failed");
    return;
  }
  
  Serial.print("Resolved IP: ");
  Serial.println(resolvedIP);

  // server address, port and URL
  socketIO.begin("https://loft.deno.dev", 80);

  socketIO.setReconnectInterval(10000);

  // event handler
  socketIO.onEvent(socketIOEvent);
}

void loop() // run over and over again
{
  socketIO.loop();
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  // if (GPSECHO)
  //   if (c)
  //     Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived())
  {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    // Serial.print(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return;                       // we can fail to parse a sentence in which case we should just wait for another
  }

  // approximately every 1/4 seconds or so, print out the current stats
  if (millis() - timer > timerInterval)
  {
    timer = millis(); // reset the timer

    // Check how connected we are to GPS Satellites
    Serial.print("Fix: ");
    Serial.print((int)GPS.fix);
    Serial.print(" quality: ");
    Serial.println((int)GPS.fixquality);
    if (GPS.fix)
    {

      Serial.println("Sending data to server...");
      // Serial.print("Location: ");
      // Serial.print(GPS.latitude, 4);
      // Serial.print(GPS.lat);
      // Serial.print(", ");
      // Serial.print(GPS.longitude, 4);
      // Serial.println(GPS.lon);
      // Serial.print("Speed (knots): ");
      // Serial.println(GPS.speed);
      // Serial.print("Angle: ");
      // Serial.println(GPS.angle);
      // Serial.print("Altitude: ");
      // Serial.println(GPS.altitude);

      // Set up JSON to send to server
      DynamicJsonDocument json(1024);
      JsonArray array = json.to<JsonArray>();
      // Add event name
      array.add("pigeon");
      JsonObject params = array.createNestedObject();

      params["latitude"] = GPS.latitude;
      params["longitude"] = GPS.longitude;
      params["altitude"] = GPS.altitude;
      params["speed"] = GPS.speed;
      params["angle"] = GPS.angle;

      String output;
      serializeJson(json, output);

      // Debug the JSON
      Serial.println(output);

      socketIO.sendEVENT(output);
    }
  }
}