#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

#define SIGNAL_TIMEOUT 1000  // This is signal timeout in milli seconds. We will reset the data if no signal

// defining light pins
#define head_light_pin 4
#define back_light_pin 5

unsigned long lastRecvTime = 0;

typedef struct PacketData
{
  byte throttle;          //ch1
  byte steering;          //ch2
  bool head_light;        //head light
  bool back_light;        //back light
  
}PacketData;
PacketData receiverData;

Servo ch1;  //steering     
Servo ch2;  //throttle     

//Assign default input received values
void setInputDefaultValues()
{
  // The middle position for joystick. (254/2=127)
  receiverData.throttle = 129;
  receiverData.steering = 127;
  // Default Value for Car Lights
  receiverData.head_light = 1;  //true or High
  receiverData.back_light = 1;  //true or High    
}

void mapAndWriteValues()
{
  ch2.write(map(receiverData.throttle, 0, 254, 0, 180));    //esc
  ch1.write(map(receiverData.steering, 0, 254, 0, 180));    //servo

  digitalWrite(head_light_pin, receiverData.head_light);  //head light
  digitalWrite(back_light_pin, receiverData.back_light);  //back light
}
// callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) 
{
  if (len == 0)
  {
    return;
  }
  memcpy(&receiverData, incomingData, sizeof(receiverData));

  String inputData ;
  inputData = inputData + "values " + receiverData.throttle + "  " + receiverData.steering + " " + receiverData.head_light + " " + receiverData.back_light  ;
  Serial.println(inputData);

  mapAndWriteValues();  
  lastRecvTime = millis(); 
}

void setUpPinModes()
{
  //Car Steerig(ch1)&Throttle(ch2) Pin Setup
  ch1.attach(12, 1000, 2000);       //Pins-> ESP32-25    ESP8266-12
  ch2.attach(13, 1000, 2000);       //Pins-> ESP32-26    ESP8266-13
  //Car Lights pin setup
  pinMode(head_light_pin, OUTPUT);
  pinMode(back_light_pin, OUTPUT);

  setInputDefaultValues();
  mapAndWriteValues();
}

void setup() 
{
  setUpPinModes();
 
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}
 


void loop()
{
  //Check Signal lost.
  unsigned long now = millis();
  if ( now - lastRecvTime > SIGNAL_TIMEOUT ) 
  {
    setInputDefaultValues();
    mapAndWriteValues();  
  }
}