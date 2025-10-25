
#include <QMC5883LCompass.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include "mbedtls/md.h"
#include "esp_system.h"
#include <Wire.h>
#include <MPU9250_asukiaaa.h>

QMC5883LCompass compass;
MPU9250_asukiaaa mySensor;
AsyncUDP udp;
Preferences pref;
// Set WiFi credentials
char packetBuffer[255];
char ret[33];
String name,pass;
int rando;
IPAddress ip;
uint16_t udpPort;
bool connection=false;
String UUID ="2a6f766c-df2c-47bb-8224-d5b8edddbc7e";
  
void setup() {
  // Setup serial port
  Serial.begin(9600);
  while(!Serial){};
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  compass.init();
  rando=10000+esp_random()%90000;
  Serial.println();
  pref.begin("name-pass",false);
  // pref.putString("name","meow_me7");
  // pref.putString("pass","idk");
  name=pref.getString("name","");
  pass=pref.getString("pass","");
  while(wm.autoConnect(name.c_str())==false){delay(5000);}
  // Begin WiFi
  // WiFi.begin(WIFI_SSID, WIFI_PASS);
  pinMode(2,OUTPUT);
  // Connecting to WiFi...
  // Serial.print("Connecting to ");
  // Serial.print(WIFI_SSID);
  // Loop continuously while WiFi is not connected
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(2,HIGH);
    delay(100);

    digitalWrite(2,LOW);
    delay(100);
    Serial.print(".");
  }
  
    digitalWrite(2,HIGH);
  // Connected to WiFi
  // Serial.println();
  // Serial.print("Connected! IP address: ");
  // Serial.println(WiFi.localIP());
  // udp.begin(5000);
  // Serial.printf("UDP Client : %s:%i \n", WiFi.localIP().toString().c_str(), 5000);
  
}
  
void loop() {
  // Serial.println(connection);
  while(connection==0){
    if(udp.listen(5555)){
      udp.onPacket([](AsyncUDPPacket p){
        String dat=(const char*)p.data();
        dat[p.length()] = '\0';
        if(dat.startsWith("Glove Thingi ping")){
          ip=p.remoteIP();
          uint16_t udpPort = 5555;
          char data[100] ;//= "Glove Thingi response Name:";
          sprintf(data,"Glove Thingi response Name:%s Key=%d",name,rando);
          udp.writeTo((uint8_t *)data,strlen(data),ip,udpPort);
        }
        if(dat.startsWith("Glove Thingi auth") && p.length()==55 && strcmp(dat.substring(23,55).c_str(),computeMD5FromString(pass.c_str()+String((std::to_string(rando)).c_str())))==0){
          IPAddress ip=p.remoteIP();
          udpPort = 5555;
          char data[100] ;//= "Glove Thingi auth Pass:1ba4bd159b11a825ef4e4b8a4d0a2b72"
          sprintf(data,"Glove Thingi response Name:%s sucessfully connected",name);
          udp.writeTo((uint8_t *)data,strlen(data),ip,udpPort);
          connection=true;
          rando=10000+esp_random()%90000;
        }
        if(dat.startsWith("Glove Thingi override") && p.length()>65 && strcmp(dat.substring(27,59).c_str(),computeMD5FromString(UUID.c_str()+String((std::to_string(rando)).c_str())))==0){
          IPAddress ip=p.remoteIP();
          udpPort = 5555;
          char data[100];//= "Glove Thingi override UUID:e98f70bf38dd94fb86cf7e344563dec9 Pass:idk"
          pref.putString("pass",dat.substring(65,p.length()-1));
          sprintf(data,"Glove Thingi response Pass Changed sucessfully",name);
          udp.writeTo((uint8_t *)data,strlen(data),ip,udpPort);
  // name=pref.getString("name","");
          pass=pref.getString("pass","");
          rando=10000+esp_random()%90000;
        }
        if(dat.startsWith("Glove Thingi name") && p.length()>61 && strcmp(dat.substring(23,55).c_str(),computeMD5FromString(UUID.c_str()+String((std::to_string(rando)).c_str())))==0){
          IPAddress ip=p.remoteIP();
          udpPort = 5555;
          char data[100];//= "Glove Thingi name UUID:e98f70bf38dd94fb86cf7e344563dec9 Name:idk"
          pref.putString("name",dat.substring(61  ,p.length()-1));
          sprintf(data,"Glove Thingi response Name Changed sucessfully",name);
          udp.writeTo((uint8_t *)data,strlen(data),ip,udpPort);
          name=pref.getString("name","");
  // pass=pref.getString("pass","");
          rando=10000+esp_random()%90000;
        }
      });
    }
  }

  Wire.begin(21, 22);      // SDA, SCL
  Wire.setClock(100000);   // 100 kHz for stability

  mySensor.setWire(&Wire); // attach Wire object

  mySensor.beginAccel();   // no return value
  mySensor.beginGyro();    // no return value
  delay(2000);
  char data[200];
  while(1){
      mySensor.accelUpdate();
      mySensor.gyroUpdate();

      // Print accelerometer
      sprintf(data,"Glove Thingi values AX:%.3lf AY:%.3lf AZ:%.3lf\nGX:%.3lf GY:%.3lf GZ:%.3lf\nMX:%.3lf MY:%.3lf MZ:%.3lf",mySensor.accelX(),mySensor.accelY(),mySensor.accelZ(),mySensor.gyroX(),mySensor.gyroY(),mySensor.gyroZ(),compass.getX(),compass.getY(),compass.getZ());
      udp.writeTo((uint8_t *)data,strlen(data),ip,udpPort);
      delay(17);
  }
  // put your main code here, to run repeatedly:
  
}

char* computeMD5FromString(const String &str){
    std::vector<uint8_t> result(16);
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_MD5;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)str.c_str(), str.length());
    mbedtls_md_finish(&ctx, result.data());
	  mbedtls_md_free(&ctx);
    for (int i = 0; i < 16; i++){
      sprintf(ret+i*2,"%02x",result.data()[i]);
    }
    ret[32]='\0';

    return ret;
}