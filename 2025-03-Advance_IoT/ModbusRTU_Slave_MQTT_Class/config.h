#ifndef CONFIG_H
#define CONFIG_H

#define Pinled1 41              // LED for Detected the Publish data
#define Pinled2 42              // LED for Connection Internet
#define rsRx 18                 // Pin for Serial RS232/RS485 UART Rx 18
#define rsTx 17                 // Pin for Serial RS232/RS485 UART Tx 17
#define SaveDisconnectTime 1000 // Time im ms for save disconnection


char *topic_pub_1 = "data"; // data/gmma/demo/A01
char *topic_pub_2 = "status";

/*--------- Variable config ---------*/
const uint8_t num_got_data = 70;
uint16_t got_data[num_got_data];

String status;
String prv_status;
uint16_t bkr_connect, modb_check;
uint16_t query_check1, query_check2;
uint16_t query_temp1, query_temp2

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

#endif