#include "CurrentRecorder.h"
#include "pages.h"

void handleRoot()
{
  sprintf(configMsg
         ,rootFmt
         ,lastCL17Reading
         ,lastPumpSpeed
         );
  server.send(200, "text/html", configMsg);
}

void handleConfig()
{
  sprintf(configMsg
         ,configFmt
         ,__DATE__
         ,__TIME__
         ,configData.ssid
         ,configData.pass
         ,configData.captive_ssid
         ,configData.captive_pass
         ,configData.sf
         ,configData.timeoutVal
         ,configData.timeoutsToReboot
         ,(configData.monitorMode)?"checked":""
         ,(configData.monitorMode)?"true":"false"
         ,configData.dnsName);
  server.send(200, "text/html", configMsg);
}

void reboot()
{
  ESP.restart();
}

void handleSet()
{
  strcpy(configData.ssid,server.arg("ssid").c_str());
  strcpy(configData.pass,server.arg("pass").c_str());
  strcpy(configData.captive_ssid, server.arg("captive_ssid").c_str());
  strcpy(configData.captive_pass, server.arg("captive_pass").c_str());
  configData.sf=atoi(server.arg("sf").c_str());
  configData.timeoutVal=atoi(server.arg("timeout_val").c_str());
  configData.timeoutsToReboot=atoi(server.arg("tt_reboot").c_str());
  configData.monitorMode=(0==strcmp("true",server.arg("monmode").c_str()));
  strcpy(configData.dnsName,server.arg("dns_name").c_str());

  nvs_handle handle;
  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  res = nvs_set_blob(handle, "cur_rec_cfg", &configData, sizeof(configData));
  Serial.printf("nvs_set_blob %i\n",res);
  nvs_commit(handle);
  nvs_close(handle);
  
  server.send(200, "text/html", 
    "<html><head></head><body onload=\"location.replace('/config');\"></body></html>\n");
}

byte speedMsg[16];

void handleSend()
{
  lastPumpSpeed=atoi(server.arg("pumpSpeed").c_str());
  speedMsg[0]='P';
  unsigned short*ps=(unsigned short *)&speedMsg[1];
  *ps=lastPumpSpeed;
  Serial.printf("Sending pump speed msg %u\n",lastPumpSpeed);
  gp.sendData(speedMsg,3);
  gp.handler();
  server.send(200, "text/html", 
    "<html><head></head><body onload=\"location.replace('/');\"></body></html>\n");
}

void eepromSetup()
{
  nvs_handle handle;

  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  size_t sz=sizeof(configData);
  res = nvs_get_blob(handle, "cur_rec_cfg", &configData, &sz);
  Serial.printf("nvs_get_blob %i; size %i\n",res,sz);
  nvs_close(handle);
  
  Serial.printf("ssid=%s\npass=%s\ncaptive_ssid=%s\ncaptive_pass=%s\nSF=%i\ntimeoutVal=%i\ntimeoutsToReboot=%i\n"
               ,configData.ssid
               ,configData.pass
               ,configData.captive_ssid
               ,configData.captive_pass
               ,configData.sf
               ,configData.timeoutVal
               ,configData.timeoutsToReboot);
}

long timeToConnect=0;

void webServerSetup()
{
  server.on("/",handleRoot);
  server.on("/send",handleSend);
  server.on("/config",handleConfig);
  server.on("/set",handleSet);
  server.on("/reboot",reboot);
  server.on("/ota", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    timeToConnect=millis()+3600000L;
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}

void wifiSTASetup(const char*ssid, const char*password)
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  Serial.println("");

  long wifiTimeOut=millis()+30000l;
  // Wait for connection
  while ((WiFi.status() != WL_CONNECTED) && (millis()<wifiTimeOut)) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi STA setup timed out");
  }
  else
  {
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void displayIPs(int x, int y, boolean fSerialPrint)
{
  char rgIPTxt[32];

  IPAddress myIP = WiFi.localIP();
  if (myIP[0]==0)
  {
    reboot();
  }
  sprintf(rgIPTxt,"%u.%u.%u.%u",myIP[0],myIP[1],myIP[2],myIP[3]);
  

  factory_display.clear();
  factory_display.drawStringMaxWidth(x, y, 128, rgIPTxt);

  factory_display.display();

  if (fSerialPrint)
  {
    Serial.print("Local IP address: ");
    Serial.println(rgIPTxt);
  }
}

void wifiAPSetup()
{
  WiFi.mode(WIFI_STA);
  wifiSTASetup(configData.ssid,configData.pass);

  displayIPs(0,0,true);
  
  telnetServer.begin();
  telnetClient=telnetServer.available();
}

void loraSend(byte *rgch,int len)
{
  delay(100);
  Serial.printf("lora sending %i bytes ",len);
  for (int i=0;i<len;i++)
  {
    Serial.printf(" %02x",rgch[i]);
  }
  Serial.println();
  LoRa.beginPacket();
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST);
  for (int i=0;i<len;i++)
  {
    LoRa.write(*rgch++);
  }
  LoRa.endPacket();
  LoRa.receive();
}

void onLoRaReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  Serial.printf("lora receive %i bytes ",packetSize);
  if (packetSize > 20) return;

  byte buff[100];
  for (int i=0;LoRa.available();i++)
  {
    buff[i]=LoRa.read();
    buff[i+1]=0;
  }
  for (int i=0;i<packetSize;i++)
  {
    Serial.printf(" %02x",buff[i]);
  }
  Serial.println();
  gp.processRecv((void*)buff,packetSize);
  LoRa.receive();
}

void logger(const char *msg)
{
  Serial.print(msg);
}

byte msg[100];

char dataTranslated[100];

void translate(byte *data,int len)
{
  unsigned *pi=(unsigned*)(&data[1]);
  unsigned short *ps=(unsigned short*)(pi);
  if (data[0]=='c') // current is flowing
  {
    sprintf(dataTranslated,"current is flowing %i %i\n",pi[0],pi[1]);
  } else if (data[0]=='n') // no current flowing
  {
    sprintf(dataTranslated,"no current flowing %i %i\n",pi[0],pi[1]);
  } else if (data[0]=='r') //raw tank reading 
  {
    sprintf(dataTranslated,"raw tank reading %i\n",*ps);
  } else if (data[0]=='T') //raw turbidity reading 
  {
    sprintf(dataTranslated,"Turbidity reading %i\n",*ps);
  } else if (data[0]=='W') // WATER USAGE VERY HIGH
  {
    sprintf(dataTranslated,"WATER USAGE VERY HIGH\n");
  }
  else if (data[0]=='C') //raw chlorine reading
  {
    lastCL17Reading=*ps;
    sprintf(dataTranslated,"CL17 Chlorine reading (raw) %i\n",lastCL17Reading);
  }
  else if (data[0]=='P') // pump setting
  {
    lastPumpSpeed=*ps;
    sprintf(dataTranslated,"Pump speed %i\n",lastPumpSpeed);
  }
}

void gotData(byte *data,int len)
{
  translate(data,len);
  Serial.println((const char *)dataTranslated);
  if (telnetClient)
  {
    telnetClient.print((const char*)dataTranslated);
    telnetClient.flush();
  }
  watchdog=millis();
  timeoutcount=0;
}

void connected()
{
  Serial.println("connected");
}

void disconnected()
{
  Serial.println("disconnected");
}

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

void setup() {
  Serial.begin(115200);
  Serial.println("in setup");
  
  Heltec.begin(false /*DisplayEnable Enable*/, 
               true /*Heltec.LoRa Disable*/, 
               true /*Serial Enable*/, 
               true /*PABOOST Enable*/, 
               BAND /*long BAND*/);

  Serial.println("after Heltec.begin");
  
  while (Serial.available())
  {
    Serial.read();
  }

  factory_display.init();
  //factory_display.flipScreenVertically();
  factory_display.setFont(ArialMT_Plain_10);
  factory_display.setTextAlignment(TEXT_ALIGN_LEFT);
  factory_display.setLogBuffer(5, 64);
  factory_display.clear();
  factory_display.drawStringMaxWidth(0, 0, 128, "about to set up LoRa");
  factory_display.display();
  

  eepromSetup();
  
  LoRa.setSpreadingFactor(configData.sf);
//  LoRa.setSpreadingFactor(7);
  LoRa.receive();

  Serial.println("Heltec.LoRa init succeeded.");

  wifiAPSetup();

  webServerSetup();
  
  Serial.println("back in setup after WiFi setup");

  MDNS.begin(configData.dnsName);
  MDNS.addService("http", "tcp", 80);
  
  watchdog=millis();

  gp.setSendMethod(&loraSend);
  gp.setOnReceive(&gotData);
  gp.setOnConnect(&connected);
  gp.setOnDisconnect(&disconnected);
  gp.setLogMethod(&logger);
  gp.setTimeout(2000);
  gp.setMonitorMode(configData.monitorMode); 
  gp.start();
}

byte buff[128];
int x=0,y=0;

void loop() 
{
  buff[0]=0;
  byte *pb=buff;
  if (telnetClient)
  {
    while (telnetClient.available())
    {
      *pb++=telnetClient.read();
      *pb=0;
    }
    if (buff[0])
    {
      gp.sendData(buff,strlen((const char*)buff));
    }
    if (configData.timeoutVal && (millis()-watchdog > (1000L*configData.timeoutVal)))
    {
      telnetClient.print("timeout\r\n");
      watchdog=millis();
      if (configData.timeoutsToReboot && (timeoutcount++ >= configData.timeoutsToReboot))
      {
        Serial.println("rebooting too many timeouts");
        reboot();
      }
    }
  }
  else if ((millis()-watchdog) > 5000)
  {
    watchdog=millis();
    telnetClient=telnetServer.available();
    if (telnetClient)
    {
      telnetClientObtained=true;
      Serial.println("telnetClient obtained");
      telnetClient.println("Hello telnet");
    }
    else if (telnetClientObtained && (WiFi.status() != WL_CONNECTED))
    {
      Serial.println("Lost telnet connection ... rebooting");
      reboot();
    }
  }
  server.handleClient();

  onLoRaReceive(LoRa.parsePacket());
  gp.handler();

  if ((millis()%100)==0)
  {
    x = (x+1)%80;
    y = (y+1)%60;
    displayIPs(x,y,false);
  }
}
