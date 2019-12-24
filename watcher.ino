//#define TEST
//#define CALIB
//#define LOG

#ifndef TEST
#include <ZMPT101B.h>
#include "CTBot.h"
#include <EEPROM.h>
CTBot myBot;



String ssid  = "****"    ; // REPLACE mySSID WITH YOUR WIFI SSID
String pass  = "*****"; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "****:******"   ; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN

int64_t id_warning;
byte a1,a2,a3,a4;
int addr = 15;
float volt;
byte status = 0;
int counter;
#define COUNTER_MAX 4
int counterCheck;
#define COUNTER_CHECK_MAX 200
ZMPT101B ac_sensor(A0,50);
#endif

void setup() {
    ESP.wdtEnable(3000);
    Serial.begin(74880);
    #ifndef TEST
      #ifdef CALIB
      ac_sensor.calibrate(220);
      #else
      EEPROM.begin(512);
      a1 = EEPROM.read(addr);
      a2 = EEPROM.read(addr+1);
      a3 = EEPROM.read(addr+2);
      a4 = EEPROM.read(addr+3);
      status = EEPROM.read(addr+4);
      if (status == 0xFF)
        status = 0;
      id_warning = a1 + a2*0x100   + a3*0x100*0x100+ a4*0x100*0x100*0x100;
      Serial.println("Starting TelegramBot...");
      Serial.println(uint64_to_string(id_warning));
      Serial.println("Status " + String(status));

      myBot.setMaxConnectionRetries(2);
      myBot.wifiConnect(ssid, pass);
      myBot.setTelegramToken(token);
      if (myBot.testConnection()) {
        Serial.println("\ntestConnection OK");
        ESP.wdtFeed();
      } else {
        Serial.println("\ntestConnection NOK");
        delay(10000);
        ESP.reset();
      }

      ac_sensor.set_a_b(7.45, -2509);
      #endif
    #endif
}

void setStatus(byte s){
  status = s;
  Serial.println("Status " + String(status));
  EEPROM.write(addr+4, status);
  EEPROM.commit();
}

void loop() {
  #ifdef TEST
    Serial.println(analogRead(A0));
  #else
    #ifndef CALIB
      if (counterCheck > COUNTER_CHECK_MAX){
          counterCheck = 0;
          if (!myBot.testConnection()){
              Serial.println("Reset!");
              delay(1000);
              ESP.reset();
          }
      } else
          counterCheck++;
      volt = ac_sensor.get_ac();
      if (volt <0)
        volt  = 0;
      #ifdef LOG
        Serial.println("Volt" +String(volt)+" counter "+String(counter)+" Status "+String(status));
      #endif
      if (status == 1){
        
        if (volt < 100) {
          counter++;
          if ((counter >= COUNTER_MAX)){
            setStatus(0);
            myBot.sendMessage(id_warning, "Пропало 220!");
            ESP.wdtFeed();
          }
        } else
          counter = 0;
      }
      if (status == 0){
        if (volt > 100) {
          counter++;
          if ((counter >= COUNTER_MAX)){
            setStatus(1);
            myBot.sendMessage(id_warning, "Восстановилось 220!");
            ESP.wdtFeed();
          } 
        } else
          counter = 0;
      }
      TBMessage msg;

      if (myBot.getNewMessage(msg)) {
        ESP.wdtFeed();
        if (msg.text == "Save"){
            id_warning = msg.sender.id;
            a1 = id_warning;
            a2 = id_warning/0x100;
            a3 = id_warning/0x100/0x100;
            a4 = id_warning/0x100/0x100/0x100;
            Serial.println(uint64_to_string(id_warning));
            EEPROM.write(addr, a1);
            EEPROM.write(addr+1, a2);
            EEPROM.write(addr+2, a3);
            EEPROM.write(addr+3, a4);
            EEPROM.commit();
            myBot.sendMessage(msg.sender.id, "Ok");
            return;
        }
        if ((msg.text == "?") || (msg.text == "/t")){
            myBot.sendMessage(msg.sender.id, "V"+String(volt)+" \n"+(status ==1 ? "220 есть" : "220 нету"));  
            return;
        }
        if (msg.text == "Clear"){
            setStatus(0);
            myBot.sendMessage(msg.sender.id, "V"+String(volt)+" \n"+(status ==1 ? "220 есть" : "220 нету"));  
            return;
        }
        myBot.sendMessage(msg.sender.id, "Команды : Save, Clear, ?, /t");
      }
      delay(5000);
    #endif
  #endif
}

char *uint64_to_string(uint64_t input) {
    static char result[21] = "";
    memset(&result[0], 0, sizeof(result));
    char temp[21] = "";
    char c;
    uint8_t base = 10;

    while (input)  {
        int num = input % base;
        input /= base;
        c = '0' + num;

        sprintf(temp, "%c%s", c, result);
        strcpy(result, temp);
    } 
    return result;
}
