#include <ModbusRtu.h>
#include "HardwareSerial.h"

Modbus slave(1 /*Slave ID (Address)*/, Serial1, 0 /*(ใช้โปรโตคอล Modbus แบบ Binary)*/);

const int max_data = 60;
uint16_t got_data[max_data];

String def_tb[][5] = {
  // name||address||type||value||prv_value
  // type for separate detail of data
  { "RUN", "1", "1", "", "" },      //Status1
  { "STOP", "2", "1", "", "" },     //Status2
  { "ALARM", "3", "1", "", "" },    //Status3
  { "Alarm1", "4", "2", "", "" },   //Status4
  { "Alarm2", "5", "2", "", "" },   //Status5
  { "Alarm3", "6", "2", "", "" },   //Status6
  { "Alarm4", "7", "2", "", "" },   //Status7
  { "Alarm5", "8", "2", "", "" },   //Status8
  { "Status1", "9", "3", "", "" },  //Data production
  { "Status2", "10", "3", "", "" },
  { "Status3", "11", "3", "", "" },
  { "Status4", "12", "3", "", "" },
  { "Status5", "13", "3", "", "" },
};


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 18, 17);
  xTaskCreatePinnedToCore(modbus_task, "Task1", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(check_task, "Task2", 10000, NULL, 2, NULL, 0);
  slave.start();
}

void modbus_task(void* parameter) {
  while (1) {
    bool change_1 = false;
    /*collect data to the table*/
    for (int i = 0; i < sizeof(def_tb) / sizeof(def_tb[0]); i++) {
      def_tb[i][3] = got_data[(def_tb[i][1].toInt()) - 1];
    }
    // Serial.printf("modbus_task Stack:%d\n",uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void check_task(void* parameter) {
  
  while (1) {
    bool change_1 = false;
    // check data type 2 is changed
    for (int i = 0; i < sizeof(def_tb) / sizeof(def_tb[0]); i++) {
      if (def_tb[i][2] == "2") {
        if (def_tb[i][3] != def_tb[i][4]) {
          change_1 = true;
        }
      }
    }
    if (change_1) {
      for (int k = 0; k < sizeof(def_tb) / sizeof(def_tb[0]); k++) {
        if (def_tb[k][2] == "2") {
          def_tb[k][4] = def_tb[k][3];
        }
      }
      print_data();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void print_data() {
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 5; j++) {
      Serial.print(def_tb[i][j]);  // พิมพ์ค่าแต่ละตัว
      Serial.print(" ");           // เพิ่มช่องว่างเพื่อให้อ่านง่าย
    }
    Serial.println();  // ขึ้นบรรทัดใหม่เมื่อจบหนึ่งแถว
  }
  Serial.println("/****************************/");
}



void loop() {
  slave.poll(got_data, max_data);
}
