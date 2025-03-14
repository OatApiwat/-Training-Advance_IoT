#include "rtuOAT.h"
/*------------------- Data list -------------------*/
// rtuOAT readData("WiFi_name", "Password", "Mqtt_server", Mqtt_port,"/department/process/","machine_number", slaveId, Serial1,"IP_Address","Gate_way","Subnet_mask");
/*-------------------------------------------------*/

rtuOAT readData("MIC_Iot", "Micdev@2024", "192.168.0.127", 1883,"/mic/test/","mc_01", 1, Serial1,"192.168.0.150","192.168.0.1","255.255.255.0"); //MIC are test

void setup() {
  readData.init();
  readData.start();
}

void loop() {
  readData.run();
}