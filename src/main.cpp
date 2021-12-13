#include <WiFi.h>
#include <WiFiMulti.h>

WiFiMulti wifiMulti;

// how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1
const char *ssid = "PTCL-BBC";
const char *password = "37581225";

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

#define telnet Serial

void setup()
{
  Serial.begin(115200);
  Serial.println("\nConnecting");

  wifiMulti.addAP(ssid, password);

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--)
  {
    if (wifiMulti.run() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else
    {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }

  // start UART and the server
  telnet.begin(9600);
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
}

void loop()
{
  uint8_t i;
  if (wifiMulti.run() == WL_CONNECTED)
  {
    // check if there are any new clients
    if (server.hasClient())
    {
      for (i = 0; i < MAX_SRV_CLIENTS; i++)
      {
        // find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected())
        {
          if (serverClients[i])
            serverClients[i].stop();
          serverClients[i] = server.available();
          if (!serverClients[i])
            Serial.println("available broken");
          Serial.print("New client: ");
          Serial.print(i);
          Serial.print(' ');
          Serial.println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS)
      {
        // no free/disconnected spot so reject
        server.available().stop();
      }
    }
    // check clients for data
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      if (serverClients[i] && serverClients[i].connected())
      {
        if (serverClients[i].available())
        {
          // get data from the telnet client and push it to the UART
          while (serverClients[i].available())
            telnet.write(serverClients[i].read());
        }
      }
      else
      {
        if (serverClients[i])
        {
          serverClients[i].stop();
        }
      }
    }
    // check UART for data
    if (telnet.available())
    {
      size_t len = telnet.available();
      uint8_t sbuf[len];
      telnet.readBytes(sbuf, len);
      // push UART data to all connected telnet clients
      for (i = 0; i < MAX_SRV_CLIENTS; i++)
      {
        if (serverClients[i] && serverClients[i].connected())
        {
          serverClients[i].write(sbuf, len);
          delay(1);
        }
      }
    }
  }
  else
  {
    Serial.println("WiFi not connected!");
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      if (serverClients[i])
        serverClients[i].stop();
    }
    delay(1000);
  }
}