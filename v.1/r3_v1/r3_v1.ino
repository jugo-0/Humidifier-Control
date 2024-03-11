#include <DHT.h>
#define DHT22PIN 2
#define relay 3
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
int relay_on_500ms;
boolean relay_aux;
boolean low_level_ilock;

unsigned long curr_time;
unsigned long tstart_dhtpoll, tstart_serialprint, tstart_humiditycheck, relay_on_time;
const unsigned long t_dhtpoll = 500, t_serialprint = 500, t_humiditycheck = 500; 

void setup() {
  Serial.begin(9600);
  dht22.begin();

  pinMode(relay, OUTPUT);
  pinMode(thermistor, INPUT);
  pinMode(relay_aux_out, OUTPUT);
  pinMode(relay_aux_in, INPUT_PULLUP);

  digitalWrite(relay, HIGH);
  digitalWrite(relay_aux_out, LOW);

  humidity_setpoint = 48.0;
  offset = 2.0;
  consumption_rate = 2.0; //water consumption percentage per minute of usage

  tstart_dhtpoll = millis();
  tstart_serialprint = millis();
  tstart_humiditycheck = millis();
}

void loop() {
  curr_time = millis();

  if(relay_aux != !digitalRead(relay)) {
    Serial.print("ERROR");
  }

  read_dht();
  serialprint();
  humidity_check();
  water_level_check();
}

void serialprint() {
  if (curr_time - tstart_serialprint >= t_serialprint){ 
    Serial.print("DHT22 temp: ");
    Serial.println(temperature_current);
    Serial.print("DHT22 humidity: ");
    Serial.println(humidity_current);
    Serial.print("Relay_Aux: ");
    Serial.println(relay_aux);
    // Serial.print("Tank Level: ");
    // Serial.println(tank_level);
    Serial.print("On Time: ");
    Serial.println(relay_on_time);
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
      digitalWrite(relay, LOW);
      relay_on_500ms ++;
      Serial.println("relay ON");
    }
    else if (humidity_current > (humidity_setpoint + offset)){
      digitalWrite(relay, HIGH);
      Serial.println("relay OFF");
    }
  relay_aux = !digitalRead(relay_aux_in);
}

void water_level_check() {
  relay_on_time = relay_on_500ms/2;
  tank_level = tank_reset - (consumption_rate/60)*relay_on_time;
  if (tank_level < tank_low) {
    low_level_ilock = 0;
  }
}
