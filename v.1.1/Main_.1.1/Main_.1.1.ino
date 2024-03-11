#include <DHT.h>
#include "WiFiS3.h"
#include "arduino_secrets.h"
#define DHT22PIN 2    //DHT22 pin
#define relay 3   //Relay control pin
#define thermistor 4    //internal temp thermistor pin
#define relay_aux_out 7   //out pin for relay aux function
#define relay_aux_in 8   //in pin for relay aux function

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
const IPAddress phoneip(192,0,43,10);

int status = WL_IDLE_STATUS;

DHT dht22(DHT22PIN, DHT22); //initializing DHT 

float humidity_current;   //current measured humifity
float temperature_current;    //current measured temp
float humidity_setpoint;    //desired humidity setpoint
float temp_internal;    //thermistor temp
float offset;   //humidity offset/hysteris
float consumption_rate;   //consumption rate of water
float tank_level, tank_reset = 80.0, tank_low = 15.0;   //tank level variables
int i, ping_fail_count = 0;
int relay_on_500ms;   //counts half seconds as relay is powered on
boolean relay_aux, ping_ok;    //relay aux state
boolean interlock, low_level_ilock, aux_ilock, ping_ilock;    //low level interlock

unsigned long curr_time;    //current time
unsigned long tstart_dhtpoll, tstart_serialprint, tstart_humiditycheck, relay_on_time;    //initial times for counted times
const unsigned long t_dhtpoll = 500, t_serialprint = 500, t_humiditycheck = 500;    //time periods

void setup() {
  Serial.begin(9600);
  dht22.begin();    //begin DHT

  pinMode(relay, OUTPUT);
  pinMode(thermistor, INPUT);
  pinMode(relay_aux_out, OUTPUT);
  pinMode(relay_aux_in, INPUT_PULLUP);

  digitalWrite(relay, HIGH);    //setting relay to initialize as off
  digitalWrite(relay_aux_out, LOW);   

  humidity_setpoint = 48.0;   //desired setpoint
  offset = 2.0;
  consumption_rate = 2.0; //water consumption percentage per minute of usage

  tstart_dhtpoll = millis();    //starting timing
  tstart_serialprint = millis();
  tstart_humiditycheck = millis();

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed. freeze !");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade to the WiFi USB bridge firmware. freeze !");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(5000);
  }

  printWifiStatus();
}

void loop() {
  curr_time = millis();

  serialprint();
  humidity_check();
  interlock_check();

}

void serialprint() {    //serial print function
  if (curr_time - tstart_serialprint >= t_serialprint){ 
    //Serial.print("DHT22 temp: ");
    //Serial.println(temperature_current);
    Serial.print("DHT22 humidity: ");
    Serial.println(humidity_current);
    //Serial.print("Relay_Aux: ");
    //Serial.println(relay_aux);
    Serial.print("Interlock: ");
    Serial.println(interlock);
    // Serial.print("Tank Level: ");
    // Serial.println(tank_level);
    Serial.print("On Time: ");
    Serial.println(relay_on_time);
    tstart_serialprint = curr_time;
  } 
}

void read_dht() {   //read DHT data
  if (curr_time - tstart_dhtpoll >= t_dhtpoll){ 
    temperature_current = dht22.readTemperature();
    humidity_current = dht22.readHumidity();
    tstart_dhtpoll = curr_time;
  }
}

void humidity_check() {   //Check current humidity and run output
  if (curr_time - tstart_humiditycheck >= t_humiditycheck){ 
    relay_output();

    read_dht();
    ping();
    water_level_check();

    tstart_humiditycheck = curr_time;
  }
}

void relay_output() {   //main relay output
  if (interlock){
    if(interlock && (humidity_current < (humidity_setpoint - offset))){
      digitalWrite(relay, LOW);
      relay_on_500ms ++;
      Serial.println("relay ON");
    }
    else if (humidity_current > (humidity_setpoint + offset)){
      digitalWrite(relay, HIGH);
      Serial.println("relay OFF");
    }
  }
  else {
    digitalWrite(relay, HIGH); 
    Serial.println("INTERLOCK FAIL RELAY AUX");
  }
  relay_aux = !digitalRead(relay_aux_in);
  if(relay_aux != digitalRead(relay)) {    //relay aux interlock testing
    aux_ilock = 1;
  }
  else {
    aux_ilock = 0;
  }
}

void water_level_check() {    //check water level
  relay_on_time = relay_on_500ms/2;
  tank_level = tank_reset - (consumption_rate/60)*relay_on_time;
  if (tank_level < tank_low) {
    low_level_ilock = 0;
  }
}

void interlock_check () {    //interlock
  interlock = aux_ilock && ping_ilock;
}

void ping () {  //ping phone ip
  // Ping IP
  Serial.print("Trying to ping phone on IP: ");
  Serial.println(phoneip);

  float res = WiFi.ping(phoneip, 1);

  if (res != 0) {
    Serial.print("Ping average response time: ");
    Serial.print(res);
    Serial.println(" ms");
    ping_ok = 1;
  }
  else {
    Serial.println("Timeout on IP!");
    ping_ok = 0;
  }

  if (ping_ok) {    // ping interlock 
    ping_ilock = 1;
    ping_fail_count = 0;
  }
  
  if ((!ping_ok) && (ping_fail_count <= 10)) {    //ping interlock fails if 2 failed pings
    ping_fail_count++;
  } else if ((!ping_ok) && (ping_fail_count > 10)){
    ping_ilock = 0;
    Serial.println("INTERLOCK FAIL PING");
  }
}

void printWifiStatus() {
/* -------------------------------------------------------------------------- */
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}