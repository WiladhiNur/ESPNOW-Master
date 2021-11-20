#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH THE MAC Address of your receiver 
uint8_t Slave1Address[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
uint8_t Slave2Address[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

// Define variables to store incoming readings
float incomingTemp;
float incomingLDRValue;

// Structure to receive data
typedef struct struct_message {
  int id;
  float value_in;
}struct_message;
// Structure to send data
typedef struct send_struct {
  int data;
} send_struct;

// Create a struct_message called myData
struct_message myData;

send_struct sent_toSlave1;
send_struct sent_toSlave2;
// Create a structure to hold the readings from each board
struct_message slave1;
struct_message slave2;

// Create an array with all the structures
struct_message boardsStruct[2] = {slave1, slave2};
send_struct send_data[2] = {sent_toSlave1, sent_toSlave2};

// callback when data is send
void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  log_i("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  log_i("%s", macStr);
  log_i(" send status:\t");
  if (status == ESP_NOW_SEND_SUCCESS){
    log_i("Delivery Success");
  }
  else{
    log_i("Delivery Fail");
  }
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  log_i("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", 
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  log_i("%s", macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  log_i("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data from all board
  boardsStruct[myData.id-1].value_in = myData.value_in;
  incomingTemp = boardsStruct[0].value_in;
  incomingLDRValue = boardsStruct[1].value_in;
  log_i("Temp value: %d \n", boardsStruct[0].value_in);
  log_i("LDR value: %d \n", boardsStruct[1].value_in);
}

 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSend);
   
  // register peer
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // register first peer  
  memcpy(peerInfo.peer_addr, Slave1Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add Slave1");
    return;
  }
  // register second peer  
  memcpy(peerInfo.peer_addr, Slave2Address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add Slave2");
    return;
  }

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // mapping temp value to heater using fungcion map(value, fromLow, fromHigh, toLow, toHigh)
  
  send_data[0].data = map((int)incomingTemp, 0, 80, 0, 255);

  if (incomingLDRValue < 100.0){
    send_data[1].data = 1;
  } else
  if (incomingLDRValue > 800.0){
    send_data[1].data = 0;
  }

  // send to salve1
  esp_err_t result1 = esp_now_send(Slave1Address, (uint8_t *) &send_data[0], sizeof(send_data[0]));
  if (result1 == ESP_OK) {
    log_i("Sent with success");
  }
  else {
    log_i("Error sending the data");
  }

  esp_err_t result2 = esp_now_send(Slave2Address, (uint8_t *) &send_data[1], sizeof(send_data[1]));
  if (result2 == ESP_OK) {
    log_i("Sent with success");
  }
  else {
    log_i("Error sending the data");
  }

  delay(1000);
}