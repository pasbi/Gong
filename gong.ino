#include "credentials.h"
#include "gongparam.h"
#include "parser.h"
#include "DFRobotDFPlayerMini.h"
#include "Arduino.h"
#include <WiFi.h>
#include "esp_system.h"
#include "rom/ets_sys.h"

WiFiServer server(80);
DFRobotDFPlayerMini player;

static constexpr auto ms_per_hour = UINT64_C(3600) * UINT64_C(1000) * UINT64_C(1000);
const auto wdt_timeout_us = UINT64_C(24) * ms_per_hour;  // trigger after a day
hw_timer_t* timer = nullptr;

void printDetail(const uint8_t type, const int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number "));
      Serial.print(value);
      Serial.println(F(": Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void setup_wifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void setup_player()
{
  static constexpr auto rx = 35;
  static constexpr auto tx = 32;
  Serial1.begin(9600, SERIAL_8N1, rx, tx);

  Serial.println();
  Serial.println("Initializing DFPlayer ... (May take 3~5 seconds)");

  static constexpr auto is_ack = true;
  static constexpr auto do_reset = true;
  if (!player.begin(Serial1, is_ack, do_reset)) {
    Serial.println("Unable to begin:");
    Serial.println("1.Please recheck the connection!");
    Serial.println("2.Please insert the SD card!");
    while(true) {
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println("DFPlayer Mini online.");
}

void print_website(WiFiClient& client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.println("<p>This is the WiFi Gong.<p>");
  client.println("<p>Click <a href=\"/gong&loudness=10&index=1\">here</a> to play a faint gong ...</p>");
  client.println("<p>... or <a href=\"/gong&loudness=30&index=1\">here</a> to play with maximum loudness.</p>");

  // The HTTP response ends with another blank line:
  client.println();
}

void gong(const GongParam& gong)
{
  Serial.print("Gong loudness=");
  Serial.print(gong.loudness);
  Serial.print(" index=");
  Serial.println(gong.index);
  player.volume(gong.loudness);
  player.play(gong.index);
}

void handle_line(const String& line)
{
  if (line.startsWith("GET /gong")) {
    gong(GongParam::from_map(Parser(line).params()));
  }
}

void ARDUINO_ISR_ATTR reboot()
{
  esp_restart();
}

void setup()
{
  Serial.begin(115200);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &reboot, true);
  timerAlarmWrite(timer, wdt_timeout_us, false);
  timerAlarmEnable(timer);
  setup_wifi();
  setup_player();
  gong({.loudness=10, .index=2});
}

void loop(){
 WiFiClient client = server.available();

  if (!client) {
    return;
  }

  Serial.println("New Client.");
  String current_line = "";
  while (client.connected()) {
    if (!client.available()) {
      continue;
    }
    const char c = client.read();

    if (c == '\n') {
      // if the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if (current_line.length() == 0) {
        print_website(client);
        break;
      } else {
        handle_line(current_line);
        current_line = "";
      }
    } else if (c != '\r') {
      current_line += c;
    }
  }

  if (player.available()) {
    printDetail(player.readType(), player.read());  // Print the detail message from DFPlayer to handle different errors and states.
  }

  client.stop();
  Serial.println("Client Disconnected.");
}
