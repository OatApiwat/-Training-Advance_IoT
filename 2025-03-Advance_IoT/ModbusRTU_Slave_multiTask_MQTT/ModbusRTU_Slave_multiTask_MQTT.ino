#include <ModbusRtu.h>
#include "HardwareSerial.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
/*---------------setting modbus------------------*/
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
/*---------------setting wifi------------------*/
const char* ssid = "MIC_Iot";
const char* password = "Micdev@2024";
const char* mqtt_server = "192.168.0.127";  // My IP Broker in mqtt

// ตั้งค่า IP คงที่
IPAddress staticIP(192, 168, 0, 150);  // IP ของ ESP32
IPAddress gateway(192, 168, 0, 1);      // Gateway
IPAddress subnet(255, 255, 255, 0);     // Subnet mask

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* topic_pub_1 = "Test/sent/data1";
const char* topic_pub_2 = "Test/sent/data2";

float ct_fn1, ct_fn2, ct_read;

void setup_wifi() {

  delay(10);  // delay unit ms
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      //mqttClient.subscribe(topic_sub_1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, /*rx =*/18, /*tx =*/17);
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  xTaskCreatePinnedToCore(modbus_task, "Task1", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mqtt_task, "Task2", 10000, NULL, 2, NULL, 0);
  slave.start();
}

void modbus_task(void* parameter) {
  while (1) {
    unsigned long long int start = micros();
    bool change_1 = false;
    /*collect data to the table*/
    for (int i = 0; i < sizeof(def_tb) / sizeof(def_tb[0]); i++) {
      def_tb[i][3] = got_data[(def_tb[i][1].toInt()) - 1];
    }
    ct_read = micros() - start;
    // Serial.printf("modbus_task use time:%d\n", ct_read);
    // Serial.printf("modbus_task Stack:%d\n",uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void mqtt_task(void* parameter) {

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
      StaticJsonDocument<200> T1;  // a little more than 200 bytes in the stack
      T1["data"] = 123;
      T1["rssi"] = WiFi.RSSI();
      String jsonString;
      serializeJson(T1, jsonString);        //Tranfer T1 from json to string
      publish_data(topic_pub_1, jsonString.c_str());  // ส่งค่า address ของ message
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

void publish_data(const char* topic,const char* message) {
  mqttClient.publish(topic, message);  // Assume ทำงานเสร็จ 500 ms
}

void loop() {
  slave.poll(got_data, max_data);
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}
