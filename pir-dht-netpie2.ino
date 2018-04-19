#include <DHT.h>
#include <ESP8266WiFi.h>
#include <MicroGear.h>

#define ledPin 4
#define PIR_PIN 16

const char* ssid     = "Nokia 5";
const char* password = "5aikon71";

#define APPID   "sonofftest3"
#define KEY     "dkQQx3NKdfKqzmK"
#define SECRET  "7mqFXWwj9MVHhqHvqsbp8qso1" 

#define ALIAS   "nodepir"

#define FEEDID  "sonopirtest2"
#define FEEDAPIKEY  "WJ8hEiiWLr27nyv1svuxNmrutX536rzG"
#define FEED_EVERY  15000

#define INTERVAL 15000 //minimun feed 15000 & freeboard 5000 ms
//#define FEED_INTERVAL 15000 //minimun feed 15sec = 15000
#define T_INCREMENT 200
#define T_RECONNECT 30000
#define BAUD_RATE 115200
#define MAX_TEMP 100
#define MAX_HUMID 100
#define PIR_DEALAY_TIME 5000 //delay time PIR 20s

WiFiClient client;

int timer = 0;
char str[32];  

#define DHTTYPE DHT22                    //Define sensor type
#define DHTPIN D4                        // Define sensor pin
DHT dht(DHTPIN, DHTTYPE);                //Initialize DHT sensor

float humid;
float temp;  
bool infrared, previous_infrared, p_status=0;
unsigned long trigger =0;

MicroGear microgear(client);

// when the other thing send a msg to this board
void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
    Serial.print("Incoming message --> ");
    msg[msglen] = '\0';
    Serial.println((char *)msg);
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    microgear.setAlias(ALIAS);
}

void setup() {
    dht.begin();
    pinMode(PIR_PIN,INPUT);
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(CONNECTED,onConnected);

    Serial.begin(BAUD_RATE);
    Serial.println("Starting...");

    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
}

void loop() {
    if (microgear.connected()) {
        Serial.print("#");
        microgear.loop();
        
        infrared = digitalRead(PIR_PIN);//read PIR
        if(infrared == 0){
          if(previous_infrared==1){
             p_status = 1;
              trigger = millis();
          }
          else if ((previous_infrared==0)&&(millis()-trigger < PIR_DEALAY_TIME)&&(trigger!=0)){p_status = 1;}
          else if ((previous_infrared==0)&&(millis()-trigger >= PIR_DEALAY_TIME)){p_status = 0;}
          else{p_status = 0;}
        }
        else{
          p_status = 1;
        }
        Serial.print("PIR:");
        Serial.print(infrared);
        Serial.print("\tStatus");
        Serial.println(p_status);
//        Serial.print("\tms:");
//        Serial.print(millis());
//        Serial.print("\ttrigger");
//        Serial.println(trigger);
        previous_infrared = infrared;

       
        if (timer >= INTERVAL) {
            humid = dht.readHumidity();
            temp = dht.readTemperature();  
            
            
            String data = "{\"humid\":";
            data += String(humid) ;
            data += ", \"temp\":";
            data += String(temp) ;
            data += ", \"infrared\":";
            data += String(p_status) ;
            data += "}";
            if (isnan(humid) || isnan(temp) || humid >= MAX_HUMID || temp>= MAX_TEMP) {
              Serial.println("Failed to read from DHT sensor!");
            }else{
              Serial.print("\nSending -->");  
              Serial.println((char*) data.c_str());
              
              microgear.publish("/dhtpir",data);
              if(FEEDAPIKEY!="") microgear.writeFeed(FEEDID,data,FEEDAPIKEY);
              else microgear.writeFeed(FEEDID,data); //YOUR  FEED ID, API KEY
              
            }
            timer = 0;
        } 
        else timer += T_INCREMENT;
    }
    else {
        Serial.println("connection lost, reconnect...");
        if (timer >= T_RECONNECT) {
            microgear.connect(APPID);
            timer = 0;
        }
        else timer += T_INCREMENT;
    }
    delay(T_INCREMENT);
   
}
