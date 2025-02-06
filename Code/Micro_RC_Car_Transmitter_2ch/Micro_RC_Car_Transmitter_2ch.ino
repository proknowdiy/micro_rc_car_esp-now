// 2 Channel Transmitter & Trims 
  #include <esp_now.h>
  #include <WiFi.h>
  #include <EEPROM.h> 

// define the number of bytes you want to access
  #define EEPROM_SIZE 1

// REPLACE WITH YOUR RECEIVER MAC Address
  uint8_t receiverMacAddress[] = {0x48,0xe7,0x29,0x6f,0xfd,0xdf};  //48:e7:29:6f:fd:df
// Light Buttons
  #define head_light_btn 19                      // Head Light Button  / Pin 19
  #define back_light_btn 18                      // Back Light Button / Pin 18
// Steering Trim Buttons
  #define trimbut_1 23                      // Trim button 1 / Pin 19
  #define trimbut_2 22                      // Trim button 2 / Pin 18

 byte head_light_btn_last_state;
 byte back_light_btn_last_state;
 byte head_light_state = 1;
 byte back_light_state = 1;
 
 int tvalue1 = EEPROM.read(0) * 16;        // Reading trim values from Eprom

 int throttle_offset = 0;                  //throttle offset
         
 typedef struct PacketData{
  byte throttle;
  byte steering;
  bool head_light;
  bool back_light; 
};
 PacketData data;

 esp_now_peer_info_t peerInfo;

  void ResetData() 
{
  data.throttle = 127;                      // Signal lost position 
  data.steering = 127;
  data.head_light = 1;
  data.back_light = 1;
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t ");
  Serial.println(status);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Message sent" : "Message failed");
}

  void setup()
{
  // Initializing Serial Monitor 
  Serial.begin(115200);
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  WiFi.mode(WIFI_STA);

  // Initializing ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Serial.println("Succes: Initialized ESP-NOW");
  }
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  else
  {
    Serial.println("Succes: Added peer");
  } 
  ResetData();
 
  pinMode(trimbut_1, INPUT_PULLUP); 
  pinMode(trimbut_2, INPUT_PULLUP);
  pinMode(head_light_btn, INPUT_PULLUP); 
  pinMode(back_light_btn, INPUT_PULLUP);
  head_light_btn_last_state = digitalRead(head_light_btn);
  back_light_btn_last_state = digitalRead(back_light_btn);

  tvalue1= EEPROM.read(0) * 16;
  //tvalue2= EEPROM.read(2) * 16;
}
// Joystick center and its borders 
  int Border_Map(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
  val = map(val, lower, middle, 0, 128);
  else
  val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}
  void loop()
{
// Trims and Limiting trim values 
  if(digitalRead(trimbut_1)==LOW and tvalue1 < 2520) {
    tvalue1=tvalue1+60;
    EEPROM.write(0,tvalue1/16);
    EEPROM.commit(); 
    delay (130);
  }   
  if(digitalRead(trimbut_2)==LOW and tvalue1 > 1120){
    tvalue1=tvalue1-60;
    EEPROM.write(0,tvalue1/16);
    EEPROM.commit();
    delay (130);
  }

  
// Control Stick Calibration for channels         
         
  data.throttle = Border_Map( analogRead(32),0,1975, 4095, true );   // For Bidirectional ESC
  data.steering = Border_Map( analogRead(33), 1220, tvalue1, 2735, false );     // "true" or "false" for signal direction // Center -- 1945

// Head Light Push to ON and Push to Off 
  byte head_light_btn_state = digitalRead(head_light_btn);

  if(head_light_btn_state != head_light_btn_last_state ){
    head_light_btn_last_state = head_light_btn_state;
    if(head_light_btn_state == LOW){
      if(head_light_state == HIGH){
        head_light_state = LOW;
      }
      else{
        head_light_state = HIGH;
      }
    }
  }
  data.head_light = head_light_state;

  // back Light Push to ON and Push to Off 
  byte back_light_btn_state = digitalRead(back_light_btn);

  if(back_light_btn_state != back_light_btn_last_state ){
    back_light_btn_last_state = back_light_btn_state;
    if(back_light_btn_state == LOW){
      if(back_light_state == HIGH){
        back_light_state = LOW;
      }
      else{
        back_light_state = HIGH;
      }
    }
  }
  data.back_light = back_light_state;

   
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &data, sizeof(data));
  if (result == ESP_OK) 
  {
    Serial.println("Sent with success");
  }
  else 
  {
    Serial.println("Error sending the data");
  }
  Serial.println(EEPROM.read(0));    
  
  delay(50);
}