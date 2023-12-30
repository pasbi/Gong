#include "credentials.h"
#include "gongparam.h"
#include "parser.h"
#include <WiFi.h>

WiFiServer server(80);

void setup()
{
    Serial.begin(115200);
    delay(10);
    
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

void print_website(WiFiClient& client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
  client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");

  // The HTTP response ends with another blank line:
  client.println();
}

void gong(const GongParam& gong)
{
  Serial.print("Gong loudness=");
  Serial.print(gong.loudness);
  Serial.print(" index=");
  Serial.println(gong.index);
}

void handle_line(const String& line)
{
  if (line.startsWith("GET /gong")) {
    gong(GongParam(Parser(line).params()));
  }
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
  
  client.stop();
  Serial.println("Client Disconnected.");
}
