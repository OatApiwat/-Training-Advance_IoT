#include "HardwareSerial.h"
#include "rtuOAT.h"
#include "config.h"


rtuOAT::rtuOAT(const char *ssid, const char *password, const char *mqtt_server, int mqtt_port, const char *dp_name, const char *mac_no, int slaveId, HardwareSerial &serialPort, const char *ip_address, const char *gateway_address, const char *subnet_mask)
  : wifiClient(), mqttClient(wifiClient), ssid(ssid), password(password), mqtt_server(mqtt_server), mqtt_port(mqtt_port), dp_name(dp_name), mac_no(mac_no), slaveId(slaveId), serialPort(serialPort), modbus_slave(slaveId, serialPort, 0) {
  ip.fromString(ip_address);
  gateway.fromString(gateway_address);
  subnet.fromString(subnet_mask);
}

void rtuOAT::setupWiFi() {
  //find the bast wifi
  int MinRSSI = -85;
  int bestNetworkIndex = -1;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);  // delete old config
  delay(500);             // 500ms seems to work in most cases, may depend on AP

  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();  // WiFi.scanNetworks will return the number of networks found
  if (n == 0) {
    Serial.println("no networks found");
    return;
  }
  // Find the network with the best RSSI value
  for (int j = 0; j < n; ++j) {
    if (WiFi.SSID(j) == ssid) {
      int rssi = WiFi.RSSI(j);
      if (rssi > MinRSSI) {
        MinRSSI = rssi;
        bestNetworkIndex = j;
      }
    }
  }
  // Connect to the network with the best RSSI value
  if (bestNetworkIndex != -1) {
    Serial.printf("Best AP Connection:%s, Signal: %d dBm, BSSID: %s, Channel: %d\n", WiFi.SSID(bestNetworkIndex).c_str(), WiFi.RSSI(bestNetworkIndex), WiFi.BSSIDstr(bestNetworkIndex).c_str(), WiFi.channel(bestNetworkIndex));
    // Connect to the selected AP
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password, 0, WiFi.BSSID(bestNetworkIndex));

    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting WiFi Fail,Restarting...");
      digitalWrite(Pinled2, HIGH);
      delay(100);
      digitalWrite(Pinled2, LOW);
      delay(1000);
    }
    if ((WiFi.status() == WL_CONNECTED)) {
      Serial.println("Connected to WiFi Completed");
      digitalWrite(Pinled2, HIGH);
    }
  } else {
    digitalWrite(Pinled2, HIGH);
    delay(100);
    digitalWrite(Pinled2, LOW);
    delay(500);
    Serial.println("can not find network");
  }
  randomSeed(micros());
}

void rtuOAT::reconnect() {
  if (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client";
    clientId += mac_no;
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT Broker");
      digitalWrite(Pinled1, LOW);  // Broker connected
    } else {
      printf("Failed with state %d\n", mqttClient.state());
      if (mqttClient.state() == -2) {
        digitalWrite(Pinled1, HIGH);  // Broker don't connection!!
      }
      delay(1000);
    }
  }
}

void rtuOAT::init() {
  std::vector<std::vector<String>> def_tb;
  pinMode(Pinled1, OUTPUT);  // Publish
  pinMode(Pinled2, OUTPUT);  // Connected

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, /*rx =*/rsRx, /*tx =*/rsTx);
  setupWiFi();
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address IoT Box: ");
  Serial.println(WiFi.localIP());
  mqttClient.setServer(mqtt_server, mqtt_port);

  digitalWrite(Pinled2, HIGH);
  modbus_slave.start();
}

void rtuOAT::publishMessage(char *topic, const char *message) {
  if (mqttClient.publish(topic, message)) {
    digitalWrite(Pinled1, HIGH);
    delay(100);
    digitalWrite(Pinled1, LOW);
  }
}

void rtuOAT::print_data() {
  for (int i = 0; i < 13; i++) {
    for (int j = 0; j < 5; j++) {
      Serial.print(def_tb[i][j]);  // พิมพ์ค่าแต่ละตัว
      Serial.print(" ");           // เพิ่มช่องว่างเพื่อให้อ่านง่าย
    }
    Serial.println();  // ขึ้นบรรทัดใหม่เมื่อจบหนึ่งแถว
  }
  Serial.println("/****************************/");
}

/*void loop()*/
void rtuOAT::run() {
  modbus_slave.poll(got_data, num_got_data);
  mqttClient.loop();
}

void rtuOAT::start() {
  xTaskCreatePinnedToCore(modbus_Task, "Task1", 10000, this, 5, NULL, 0);
  xTaskCreatePinnedToCore(Network_Task, "Task2", 10000, this, 4, NULL, 0);
  xTaskCreatePinnedToCore(mqtt_Task, "Task3", 10000, this, 3, NULL, 0);
  xTaskCreatePinnedToCore(broke_modbus_Task, "Task4", 10000, this, 2, NULL, 0);
  xTaskCreatePinnedToCore(esp_Task, "Task5", 10000, this, 1, NULL, 0);
}

void rtuOAT::modbus_Task(void *pvParam) {
  rtuOAT *instance = (rtuOAT *)pvParam;
  while (1) {
    // record raw data to table
    unsigned long long int start = micros();
    for (int i = 0; i < sizeof(def_tb) / sizeof(def_tb[0]); i++) {
      def_tb[i][3] = got_data[(def_tb[i][1].toInt()) - 1];
    }
    ct_read = micros() - start;
    /* interval work loop 150-180 microsec*/
    // Serial.printf("modbus_task use time:%d\n", ct_read);
    // Serial.printf("modbus_task Stack:%d\n",uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(itr_modbus));
  }
}

void rtuOAT::Network_Task(void *pvParam) {
  rtuOAT *instance = (rtuOAT *)pvParam;

  while (1) {
    /*-------- Check Internet & Server MQTT --------*/
    if ((WiFi.status() != WL_CONNECTED)) {
      digitalWrite(Pinled2, HIGH);
      delay(100);
      digitalWrite(Pinled2, LOW);
      /*reconnect wifi*/
      instance->setupWiFi();
    }
    if (!(instance->mqttClient.connected())) {
      /*reconnect mqtt*/
      instance->reconnect();
    }
    // Serial.printf("Network_Task Stack:%d\n", uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(itr_network));  // loop get value every 5 sec
  }
}

void rtuOAT::mqtt_Task(void *pvParam) {
  rtuOAT *instance = (rtuOAT *)pvParam;
  rtuOAT *dpName = (rtuOAT *)(pvParam);
  rtuOAT *macNo = (rtuOAT *)(pvParam);
  /*concat topic with department_name and machine_number*/
  char topic_pub[30];
  strcpy(topic_pub, topic_pub_1);
  strcat(topic_pub, dpName->dp_name);
  strcat(topic_pub, macNo->mac_no);
  Serial.printf("topic: %s\n",topic_pub);
  while (1) {
    unsigned long long int start = micros();
    bool change_1 = false;
    StaticJsonDocument<500> json_1; /*this value follow https://arduinojson.org/v6/assistant/#/step1*/
    // check data type 2 change
    for (int i = 0; i < sizeof(def_tb) / sizeof(def_tb[0]); i++) {
      if (def_tb[i][2] == "2") {
        if (def_tb[i][3] != def_tb[i][4]) {
          change_1 = true;
          break;
        }
      }
    }
    if (change_1) {  // data change !!!

      /*----------------- rssi value -----------------*/
      json_1["rssi"] = (float)WiFi.RSSI();

      /*----------------- Production data -----------------*/
      for (int j = 0; j < sizeof(def_tb) / sizeof(def_tb[0]); j++) {
        if (def_tb[j][2] == "2") {
          json_1[String(def_tb[j][0])] = def_tb[j][3].toFloat();
        }
      }
      /*----------------- Publish data -----------------*/
      String json_topic1;
      // char json_topic1[100];
      // Serial.println(sizeof(json_topic1));
      // Serial.println(sizeof(json_1));
      serializeJson(json_1, json_topic1);
      // size_t JsonSize = serializeJson(json_1, json_topic1);
      // instance->publishMessage(mcNo->mc_no, json_topic1.c_str());
      instance->publishMessage(topic_pub, json_topic1.c_str());
      Serial.println(json_topic1);
      for (int k = 0; k < sizeof(def_tb) / sizeof(def_tb[0]); k++) {
        if (def_tb[k][2] == "2") {
          def_tb[k][4] = def_tb[k][3];
        }
      }
      ct_fn1 = micros() - start;
    }
    // interval work loop 100-140 ms
    vTaskDelay(pdMS_TO_TICKS(itr_fnc_1));
  }
}


void rtuOAT::broke_modbus_Task(void *pvParam) {  // Check modbus,Broker alive
  rtuOAT *instance = (rtuOAT *)pvParam;
  rtuOAT *dpName = (rtuOAT *)(pvParam);
  rtuOAT *macNo = (rtuOAT *)(pvParam);
  rtuOAT *vrs_Code = (rtuOAT *)(pvParam);

  char topic_pub[30];
  strcpy(topic_pub, topic_broke_modbus);
  strcat(topic_pub, dpName->dp_name);
  strcat(topic_pub, macNo->mac_no);

  while (1) {
    unsigned long long int start = millis();
    StaticJsonDocument<200> json_4;
    String json_topic4;
    if (instance->mqttClient.connected()) {
      bkr_connect = 1;
    }
    json_4["broker"] = bkr_connect;

    query_check1 = instance->modbus_slave.getInCnt();   // Get input messages counter value and return input messages counter
    query_check2 = instance->modbus_slave.getOutCnt();  // Get transmitted messages counter value and return transmitted messages counter
    // printf("query_check1 : %d,query_temp1 : %d,query_check2 : %d,query_temp2 : %d\n", query_check1, query_temp1, query_check2, query_temp2);

    if ((start - prv_time_2) >= 5000) {  // Check data from GOT every 5s
      if ((query_check1 == query_temp1) && (query_check2 == query_temp2)) {
        modb_check = 0;
      } else {
        modb_check = 1;
      }
      prv_time_2 = start;
      query_temp1 = query_check1;
      query_temp2 = query_check2;
    }
    json_4["modbus"] = modb_check;
    // json_4["version"] = vrs_Code->vrs_code;  // code version

    if (((bkr_connect == 1) && (tigger_1 == 1)) || ((start - prv_time_1) >= (5 * (60 * 1000)))) {  // Use tigger = 1 for Publish first time And Publish data every 30 mins.
      serializeJson(json_4, json_topic4);
      instance->publishMessage(topic_pub, json_topic4.c_str());
      Serial.println(json_topic4);
      prv_time_1 = start;
      tigger_1 = 0;  // End use tigger forever until Start program again.
    }
    vTaskDelay(pdMS_TO_TICKS(itr_bro_mod));
  }
}

void rtuOAT::esp_Task(void *pvParam) {  // ESP status
  rtuOAT *instance = (rtuOAT *)pvParam;
  rtuOAT *dpName = (rtuOAT *)(pvParam);
  rtuOAT *macNo = (rtuOAT *)(pvParam);

  char topic_pub[30];
  strcpy(topic_pub, topic_esp_health);
  strcat(topic_pub, dpName->dp_name);
  strcat(topic_pub, macNo->mac_no);
  while (1) {
    unsigned long long int start = millis();
    StaticJsonDocument<200> json_5;
    String json_topic5;
    float use_heap = (1 - (esp_get_free_heap_size() / init_heap)) * 100;
    // check heap
    if (use_heap >= 20.0 && use_heap <= 40.0) {
      heap_cnt1++;
    } else if (use_heap > 40.0 && use_heap <= 60.0) {
      heap_cnt2++;
    } else if (use_heap > 60.0) {
      heap_cnt3++;
    }
    // check cpu
    float read_over = ((ct_read / ct_read_) - 1) * 100;
    if (read_over > 80) {
      ct_read_cnt++;
    }
    float fnc1_over = ((ct_fn1 / ct_fn1_) - 1) * 100;
    if (fnc1_over > 80) {
      ct_fn1_cnt++;
    }
    float fnc2_over = ((ct_fn2 / ct_fn2_) - 1) * 100;
    if (fnc2_over > 80) {
      ct_fn2_cnt++;
    }
    // float fnc3_over = ((ct_fn3 / ct_fn3_) - 1) * 100;
    // if (fnc3_over > 80) {
    //   ct_fn3_cnt++;
    // }

    if (start - prv_time >= (12 * (60 * (60 * 1000)))) {  // 12hr
      json_5["mem_use"] = use_heap;
      json_5["mem_cnt1"] = heap_cnt1;
      json_5["mem_cnt2"] = heap_cnt2;
      json_5["mem_cnt3"] = heap_cnt3;
      json_5["cpu_fn0"] = ct_read_cnt;
      json_5["cpu_fn1"] = ct_fn1_cnt;
      json_5["cpu_fn2"] = ct_fn2_cnt;
      // json_3["cpu_fn3"] = ct_fn3_cnt;

      serializeJson(json_5, json_topic5);
      instance->publishMessage(topic_pub, json_topic5.c_str());
      Serial.println(json_topic5);
      prv_time = start;
      heap_cnt1 = 0;
      heap_cnt2 = 0;
      heap_cnt3 = 0;
      ct_read_cnt = 0;
      ct_fn1_cnt = 0;
      ct_fn2_cnt = 0;
      // ct_fn3_cnt = 0;
    }
    ct_read = 0;
    ct_fn1 = 0;
    ct_fn2 = 0;
    vTaskDelay(pdMS_TO_TICKS(itr_esp));
  }
}
