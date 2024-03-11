#include <DHT.h>
#define DHT22PIN 2
#define relay 13
#define thermistor 4
#define relay_aux_out 7
#define relay_aux_in 8

DHT dht22(DHT22PIN, DHT22);

float humidity_current;
float temperature_current;
float humidity_setpoint;
float temp_internal;
float offset;
float consumption_rate;
float tank_level, tank_reset = 80.0, tank_low = 15.0;
int i;
boolean relay_aux;
boolean cumulative_state;

unsigned long curr_time;
unsigned long tstart_dhtpoll, tstart_serialprint, tstart_humiditycheck, relay_on_timestamp, relay_off_timestamp, relay_on_time;
const unsigned long t_dhtpoll = 500, t_serialprint = 500, t_humiditycheck = 500; 

void setup() {
  Serial.begin(9600);
  dht22.begin();

  pinMode(relay, OUTPUT);
  pinMode(thermistor, INPUT);
  pinMode(relay_aux_out, OUTPUT);
  pinMode(relay_aux_in, INPUT_PULLUP);

  digitalWrite(relay, LOW);
  cumulative_state = 0;
  digitalWrite(relay_aux_out, HIGH);

  humidity_setpoint = 48.0;
  offset = 2.0;
  consumption_rate = 2.0; //water consumption percentage per minute of usage

  tstart_dhtpoll = millis();
  tstart_serialprint = millis();
  tstart_humiditycheck = millis();
}

void loop() {
  curr_time = millis();

  //relay_aux = digitalRead(relay_aux_in);

  read_dht();
  //serialprint();
  humidity_check();
  water_level_check();

}

void serialprint() {
  if (curr_time - tstart_serialprint >= t_serialprint){ 
    Serial.print("DHT22 temp: ");
    Serial.println(temperature_current);
    Serial.print("DHT22 humidity: ");
    Serial.println(humidity_current);
    //Serial.println(relay_aux);
    tstart_serialprint = curr_time;
  } 
}

void read_dht() {
  if (curr_time - tstart_dhtpoll >= t_dhtpoll){ 
    temperature_current = dht22.readTemperature();
    humidity_current = dht22.readHumidity();
    tstart_dhtpoll = curr_time;
  }
}

void humidity_check() {
  if (curr_time - tstart_humiditycheck >= t_humiditycheck){ 
    relay_output();
    tstart_humiditycheck = curr_time;
  }
}

void relay_output() {
    if(humidity_current < (humidity_setpoint - offset)){
      digitalWrite(relay, HIGH);
      relay_aux = 1;
      Serial.println("relay ON");
    }
    else if (humidity_current > (humidity_setpoint + offset)){
      digitalWrite(relay, LOW);
      relay_aux = 0;
      Serial.println("relay OFF");
    }
}

void water_level_check() {
  if (relay_aux != cumulative_state) {
    cumulative_state = !cumulative_state;
    if (relay_aux) {
      relay_on_timestamp = millis(); 
    }
    if (!relay_aux) {
      relay_off_timestamp = millis();
      //relay_on_time = relay_on_time + (relay_off_timestamp - relay_on_timestamp);
    }
    Serial.print("on time seconds ");
    Serial.println(relay_on_time/1000);
  }
  if (relay_aux){
    relay_on_time = relay_on_time + (relay_off_timestamp - relay_on_timestamp);
  }
  tank_level = tank_reset - (consumption_rate/60000)*relay_on_time;
}
