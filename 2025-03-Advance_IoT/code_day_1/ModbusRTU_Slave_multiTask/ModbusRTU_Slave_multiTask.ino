#include <ModbusRtu.h>

Modbus slave(1, Serial1, 0);

const int max_data = 5;
uint16_t modbus_data[max_data];

void read_modbus(void* parameter) {
  while (true) {
    slave.poll(modbus_data, max_data);

    vTaskDelay(pdMS_TO_TICKS(10));
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
    Serial.println("---------------------DATA------------------------ \n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 18, 17);
  xTaskCreatePinnedToCore(read_modbus, "Task1", 5000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(print, "Task2", 5000, NULL, 2, NULL, 0);
  slave.start();
}

void loop() {
}
