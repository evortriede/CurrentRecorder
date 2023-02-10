#include <fsm.h>

#include <GenericProtocol.h>

#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <nvs.h>
#include <ESPmDNS.h>

WiFiServer telnetServer(23);
WiFiClient telnetClient;
WebServer server(80);
GenericProtocol gp;

typedef struct
{
  char ssid[25];
  char pass[25];
  char captive_ssid[25];
  char captive_pass[25];
  int sf;
  int timeoutVal;
  int timeoutsToReboot;
  bool monitorMode;
  char dnsName[25];
} config_data_t;

config_data_t configData={"jenanderic","jenloveseric","CurrentRec","",8,0,0,false,"CurrentRecorder"};

unsigned short lastPumpSpeed=0;
unsigned short lastCL17Reading=0;

char configMsg[4096];

long watchdog;
int timeoutcount=0;
bool telnetClientObtained=false;
