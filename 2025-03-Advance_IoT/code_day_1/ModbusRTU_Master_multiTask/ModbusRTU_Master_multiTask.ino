#include <ModbusRtu.h>

Modbus master(0, Serial1, 0);  

const int max_data = 10;
uint16_t modbus_data[max_data];

const uint8_t SLAVE_ID = 1;
const uint16_t START_ADDRESS = 0;

void read_modbus(void* parameter) {
  modbus_t telegram = {SLAVE_ID, 3, START_ADDRESS, max_data, modbus_data};
  
  while (true) {
    master.poll();
    master.query(telegram);
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void print(void* parameter) {
  while (true) {
    Serial.println("---------------------DATA------------------------ ");
    Serial.print("D0  : ");
    Serial.println(modbus_data[0]);
    Serial.print("D1  : ");
    Serial.println(modbus_data[1]);
    Serial.print("D2  : ");
    Serial.println(modbus_data[2]);
    Serial.print("D3  : ");
    Serial.println(modbus_data[3]);
    Serial.print("D4  : ");
    Serial.println(modbus_data[4]);
    Serial.print("D5  : ");
    Serial.println(modbus_data[5]);
    Serial.print("D6  : ");
    Serial.println(modbus_data[6]);
    Serial.print("D7  : ");
    Serial.println(modbus_data[7]);
    Serial.println("---------------------DATA------------------------ \n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 18, 17);
  
  xTaskCreatePinnedToCore(read_modbus, "Task1", 5000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(print, "Task2", 5000, NULL, 2, NULL, 0);
  
  master.start();
}

void loop() {
}