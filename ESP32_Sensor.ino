#include "WiFiProv.h"
#include "WiFi.h"
#include "nvs_flash.h"
#include "Adafruit_SHT31.h"
#include "Queue.h"
#include <Arduino.h>
#include <Wire.h>
#include "secrets.h"

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
  
//Bucket name
#define BUCKET "Sensor Data"
  
// Time zone
#define TZ_INFO "PST8PDT"

//Provisioning
#define ID "clim_ctrl_01"
#define POP "abcd1234"
  
// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(URL, ORG, BUCKET, TOKEN, InfluxDbCloud2CACert);
  
Point sensor("climate");

//for dummy data
float temp = 71.35;
float hmd = 83.89;
int loops = 0;
int change = 1;
boolean client_connected = false;
boolean tag_set = false;
boolean time_sync = false;
bool enableHeater = false;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

Queue<float> temperature(1000);
Queue<float> humidity(1000);
Queue<int> data_time(1000);

void client_connect(){
  if(time_sync == false){
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    time_sync = true;
  }
     // Check server connection
     while(client_connected == false){
       if (client.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
        client_connected = true;
      } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
      }
    }
}

void SysProvEvent(arduino_event_t *sys_event)
{
    switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("\nConnected IP address : ");
        Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
        client_connect();
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("\nDisconnected. Connecting to the AP again... ");
        client_connected = false;
        break;
    case ARDUINO_EVENT_PROV_START:
        Serial.println("\nProvisioning started\nGive Credentials of your access point using \" Android app \"");
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV: { 
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("\tSSID : ");
        Serial.println((const char *) sys_event->event_info.prov_cred_recv.ssid);
        Serial.print("\tPassword : ");
        Serial.println((char const *) sys_event->event_info.prov_cred_recv.password);
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_FAIL: { 
        Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
        if(sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR) 
            Serial.println("\nWi-Fi AP password incorrect");
        else
            Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");        
        break;
    }
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        Serial.println("\nProvisioning Successful");
        break;
    //case ARDUINO_EVENT_PROV_END:
        //Serial.println("\nProvisioning Ends");
        //break;
    default:
        break;
    }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.onEvent(SysProvEvent);
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, POP, ID);

  pinMode(0, INPUT_PULLUP);
  
  client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::S));

  /*Serial.println("SHT31 test");
  if(!sht31.begin(0x44)){
    Serial.println("Could not find SHT31");
  }*/

}

void loop() {
  //change heater state
  /*if (loops % 3 == 0) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);    
    
    Serial.print("Heater Enabled State: ");
    
    if (sht31.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");
  }*/
  if(digitalRead(0) == LOW){
    Serial.print("\nReset Sensor");
    nvs_flash_erase();
    ESP.restart();
  }

  //send temp and hmd
  if(loops % 30 == 0 && client_connected == true && temperature.count() >= 1){
    if(tag_set == false){ //if needed set tag
      sensor.addTag("id", ID);
      tag_set = true;
    }
    
      // Clear fields for reusing the point. Tags will remain the same as set above.
      sensor.clearFields();
  
      // Store measured value into point
      sensor.addField("temp", temperature.pop());
      sensor.addField("hmd", humidity.pop());
      sensor.setTime(data_time.pop()); 
    
      // Print what are we exactly writing
      Serial.print("Writing: ");
      Serial.println(sensor.toLineProtocol());
        
      // Write point
      if (!client.writePoint(sensor)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
      }
  }

  //read temp and hmd
  if(loops % 60 == 0){
    /*if (sht31.isHeaterEnabled())
        temp = (sht31.readTemperature() - 1.5) * (9/5) + 32;
      else
        temp = sht31.readTemperature() * (9/5) + 32;
  
      hmd = sht31.readHumidity();
      
    // Store temperature to be sent
      temperature.push(temp);
      humidity.push(hmd);

    // Print temperature to console
      if (! isnan(temp)) { // check if 'is not a number'
        Serial.print("Temp *F = "); 
        Serial.print(temp); 
        Serial.print("\t\t");
      }
      else 
        Serial.println("Failed to read temperature");
  
    // Print humidity to console
      if (! isnan(hmd)) {  // check if 'is not a number'
        Serial.print("Hum. % = "); 
        Serial.println(hmd);
      }
      else
        Serial.println("Failed to read humidity");
     */

    //dummy data 
    if(time_sync == true){
      temperature.push(temp);
      humidity.push(hmd);
      data_time.push(time(nullptr));

      temp = temp + change;
      hmd = hmd + change;
    } 
  }

  if(loops++ >= 3600){
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    loops = 0;
  }

  //dummy data
  if(loops % 300 == 0){
    change = 0 - change;
  }

  //loops++;
  //Serial.println(loops);
  delay(1000);
}

