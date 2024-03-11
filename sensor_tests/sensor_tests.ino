#include <DHT.h>
#define DHT11PIN 2
#define DHT22PIN 3

DHT dht11(DHT11PIN, DHT11);
DHT dht22(DHT22PIN, DHT22);

DHT dht[] = {
  {DHT11PIN, DHT11},
  {DHT22PIN, DHT22}
};

float humidity[2];
float temperature[2];
int i;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (auto& sensor : dht) {
    sensor.begin();
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  for (i = 0; i < 2; i++){
    temperature[i] = dht[i].readTemperature();
    humidity[i] = dht[i].readHumidity();
  }

  Serial.print("DHT11 temp: ");
  Serial.println(temperature[0]);
  
  Serial.print("DHT22 temp: ");
  Serial.println(temperature[1]);

  Serial.print("DHT11 humidity: ");
  Serial.println(humidity[0]);
  Serial.print("DHT22 humidity: ");
  Serial.println(humidity[1]);

  delay(1000);

}

