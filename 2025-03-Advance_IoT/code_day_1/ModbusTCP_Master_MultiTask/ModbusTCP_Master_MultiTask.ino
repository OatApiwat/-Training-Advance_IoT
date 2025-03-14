#include <SPI.h>
#include <EthernetENC.h>
#include <ModbusEthernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(169, 254, 234, 60);       //IP ของกล่อง
IPAddress slaveIp(169, 254, 234, 55);  // IP of the Modbus slave device
ModbusEthernet mb;

const int max_data = 10;
uint16_t modbus_data[max_data];

void read_modbus(void* parameter) {
  while (true) {
    mb.task();
    if (mb.isConnected(slaveIp)) {
      mb.readHreg(slaveIp, 0, modbus_data, max_data);
    } else {
      Serial.println("Connecting to slave...");  //ติดต่อกับ slave ไม่ได้
      mb.connect(slaveIp);                       // Attempt to connect to the slave
    }
    
    mb.connect(slaveIp);
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
  Ethernet.begin(mac, ip);
  delay(1000);

#if defined(ARDUINO_ESP32S2_DEV)
  Ethernet.init(34);  // SS Pin for SMM-002
#elif defined(ARDUINO_ESP32S3_DEV)
  Ethernet.init(10);  // SS Pin for SMM-002A
#endif

  mb.client();

  xTaskCreatePinnedToCore(read_modbus, "Task1", 5000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(print, "Task2", 5000, NULL, 2, NULL, 0);
}

void loop() {
}
