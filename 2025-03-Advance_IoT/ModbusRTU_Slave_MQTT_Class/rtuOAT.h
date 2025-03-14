#ifndef RTUOAT_H
#define RTUOAT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "ModbusRtu.h"
#include <vector>
#include <ArduinoJson.h>


class rtuOAT {
private:
  /*variable that import frome user*/
  const char* ssid;
  const char* password;
  const char* mqtt_server;
  int mqtt_port;
  const char* dp_name;
  const char* mac_no;
  PubSubClient mqttClient;

  int slaveId;
  HardwareSerial& serialPort;
  Modbus modbus_slave;

  IPAddress ip;
  IPAddress gateway;
  IPAddress subnet;
public:
  rtuOAT(const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* dp_name, const char* mac_no, int slaveId, HardwareSerial& serialPort, const char* ip_address, const char* gateway_address, const char* subnet_mask);
  void setupWiFi();
  void reconnect();
  void init();
  void publishMessage(char* topic, const char* message);
  void run();
  void start();
  void print();
  static void modbus_Task(void* pvParam);
  static void mqtt_Task(void* pvParam);
};

#endif