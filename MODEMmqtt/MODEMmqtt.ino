#include <RIC3D.h>
#include <RIC3DMODEM.h>

#define SerialMon Serial

#define SerialAT Serial3

// Module baud rate
uint32_t rate = 115200; 

// Select SIM Card (0 = right, 1 = left)
bool sim_selected = 1; 

const char apn[]      = "grupotesacom.claro.com.ar";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char mqtt_ip[] = "10.25.1.152";
const char port[] = "4099";

const char client_id[]   = "IVR4oiZ62ymacQLSb6qo"; // aca se debe poner el id del device de tdata
const double humidity[] = {
93.82894,
94.19782,
94.83262,
96.47203,
97.1187,
96.16575,
96.98019,
97.79321,
98.20112,
98.19411,
97.83999,
95.36762,
92.20724,
88.76688,
83.10312,
79.97462,
77.46256,
84.3071,
93.68196,
95.78908,
95.44552,
94.87827,
95.85379,
95.96225,
98.1573,
98.01298,
97.08777,
96.88525,
97.48621,
96.64618,
96.09441,
95.54446,
95.07294,
91.53713,
88.35643,
83.72151,
81.80693};

const double dbs[] = {
10.82894,
4.19782,
3.83262,
5.47203,
8.1187,
21.16575,
20.98019,
17.79321,
31.20112,
30.19411,
28.83999,
45.36762,
42.20724,
41.76688,
15.10312,
27.97462,
20.46256,
39.3071,
45.68196,
80.78908,
67.44552,
56.7827,
45.85379,
35.96225,
38.1573,
36.01298,
24.08777,
18.88525,
15.48621,
14.64618,
9.09441,
12.54446,
16.07294,
43.53713,
61.35643,
21.72151,
12.80693};


int humCounter = 0;
int humIndex = 0;
int noiseCounter = 0;
int noiseIndex = 0;
const double epsilon = 0.5;
const double epsilon_h = 1;

// Armar relacion entre CO y CO2 con temperatura
int doorState = 0;
int lastDoorState = 0;
int sensorValue = 0;
double humValue = 0.0;
double noiseValue = 0.0;
int co2State = 1;
int coState = 1;
int carbonDioxideSensor = 0;

float meanC = 0;
float minC = 20;
float maxC = 4;
int currentCount = 0;
float temperature = 0;
float lastTemperature = 0;
float minTemperature = 0;
float maxTemperature = 0;
float voltage = 0;
unsigned long startTime;
unsigned long currentTime;
unsigned long co2Time;
unsigned long coTime;
unsigned long fiveMinuteTime;
char str[7];
char strHum[7];
char strDoor[7];
char strConcentration[7];
char strNoise[7];

double calculateHumidity(double input) {
  srand(currentTime);
  return rand() % (int)((input + epsilon) - (input - epsilon) + 1) + (input - epsilon);
}

double calculateNoise(double input) {
  srand(currentTime);
  return rand() % (int)((input + epsilon_h) - (input - epsilon_h) + 1) + (input - epsilon_h);
}

// Fix maximum temperature lecture
float calculateTemperature(float input) {
  return 6.25 * input - 45;
}

void updateMin(float input) {
  if (input < minC)
    minC = input;
}

void updateMax(float input) {
  if (input > maxC)
    maxC = input;
}

void simulateConcentration(char* name, int offset, long* time, int* concentrationState) {
  // if (strcmp(name, "co") == 0) {
  //   SerialMon.println("\nCo: ");
  //   SerialMon.println(currentTime - *time);
  //   SerialMon.println(40000 + offset);
  //   SerialMon.println(60000 + offset);
  //   SerialMon.println(100000 + offset);
  //   SerialMon.println(500000 + offset);
  // }
  
  if (currentTime - *time < (40000 + offset)) {
    // SerialMon.println("\nHolis");
    // SerialMon.println(name);
    if (*concentrationState == 1) {
      Serial.print(name);
      Serial.print(":");
      Serial.println(*concentrationState);

      strcpy(strConcentration, "BAJO");
      PublishData(name,strConcentration);
      *concentrationState = 0;
    }
  } else if (currentTime - *time < (60000 + offset)) {
    if (*concentrationState == 0) {
      Serial.print(name);
      Serial.print(":");
      Serial.println(*concentrationState);

      strcpy(strConcentration, "ALTO");
      PublishData(name,strConcentration);
      *concentrationState = 1;
    }
  } else if (currentTime - *time < (100000 + offset)) {
    if (*concentrationState == 1) {
      Serial.print(name);
      Serial.print(":");
      Serial.println(*concentrationState);

      strcpy(strConcentration, "BAJO");
      PublishData(name,strConcentration);
      *concentrationState = 0;
    }
  } else if (currentTime - *time < (500000 + offset)) {
    if (*concentrationState == 0) {
      Serial.print(name);
      Serial.print(":");
      Serial.println(*concentrationState);
      
      strcpy(strConcentration, "ALTO");
      PublishData(name,strConcentration);
      *concentrationState = 1;
    }
  } else {
    *time = currentTime;
  }
}

void setup() 
{
  SerialMon.begin(115200);
  SerialMon.println(F("***********************************************************"));
  SerialMon.println(F(" Initializing Modem"));
  pinMode(SIM_SELECT,OUTPUT);
  digitalWrite(SIM_SELECT,sim_selected);
  SerialMon.print(" Sim selected is the one on the ");
  SerialMon.println(sim_selected?"left":"right");
  ModemTurnOff();
  ModemTurnOn();
  SerialAT.begin(rate);
  analogReference(INTERNAL2V56);
  SerialMon.println(" Opening MQTT service ");
  CreatePDPContext(apn, gprsUser,  gprsPass);
  ActivatePDPContext();
  ConnectMQTTClient(client_id, mqtt_ip, port);
  startTime = millis();
  co2Time = millis();
  coTime = millis();
  fiveMinuteTime = millis();

  pinMode(DI1, INPUT_PULLUP);
  pinMode(DI4, INPUT_PULLUP);
}

void readTemperature() {
  currentTime = millis();
  if (currentTime - fiveMinuteTime > 5000) {
    sensorValue = analogRead(AI0)/40;
    updateMin(sensorValue);
    updateMax(sensorValue);
    meanC = (meanC * currentCount + sensorValue)/(currentCount + 1);
    currentCount++;

    fiveMinuteTime = currentTime;
    temperature = calculateTemperature(meanC);
    if (abs(temperature - lastTemperature) > 10) {
      humValue = calculateHumidity(humidity[humIndex % 37]);
      dtostrf(humValue, 4, 2, strHum);
      dtostrf(temperature, 4, 2, str);
      PublishDataExtended("temp_av", str, "humidity", strHum);

      Serial.print("temp_av:");
      Serial.println(temperature);
      
      Serial.print("humidity:");
      Serial.println(humValue);
      // PublishData("temp_av",str);
    }
    lastTemperature = temperature;
  }

  if(currentTime - startTime > 30000) {
    Serial.print("temp_av:");
    Serial.println(temperature);
    
    dtostrf(temperature, 4, 2, str);
    if (temperature > 60) {
      strcpy(strConcentration, "ALTO");
      PublishData("co2",strConcentration);
      Serial.print("co2:");
      Serial.println("1");
    }
    // PublishData("temp_av",str);

    // minTemperature = calculateTemperature(minC);
    
    // dtostrf(minTemperature, 4, 2, str);
    // PublishData("temp_min",str);

    // maxTemperature = calculateTemperature(maxC);
    
    // dtostrf(maxTemperature, 4, 2, str);
    // PublishData("temp_max",str);

    humValue = calculateHumidity(humidity[humIndex % 37]);
    dtostrf(humValue, 4, 2, strHum);

    Serial.print("humidity:");
    Serial.println(humValue);

    // PublishData("humidity",strHum);
    PublishDataExtended("temp_av", str, "humidity", strHum);

    noiseValue = calculateNoise(dbs[noiseIndex % 37]);
    noiseCounter++;
    if (noiseCounter > 10) {
      noiseCounter = 0;
      noiseIndex++;
    }
    dtostrf(noiseValue, 4, 2, strNoise);

    Serial.print("noise:");
    Serial.println(noiseValue);

    PublishData("noise", strNoise);

    humCounter++;
    if (humCounter > 288) {
      humCounter = 0;
      humIndex++;
    }   
    currentCount = 0;
    startTime = millis();
  }

}

void loop() 
{

  // currentTime = millis();
  doorState = digitalRead(DI1);

  if (doorState != lastDoorState) {
    // SerialMon.println("\nCurrent sensorValue: ");
    // SerialMon.println(doorState);

    if (doorState == 0)
      strcpy(strDoor, "CERRADO");
    else 
      strcpy(strDoor, "ABIERTO");
    
    Serial.print("door:");
    Serial.println(doorState);

    PublishData("door",strDoor);

    lastDoorState = doorState;
  }
  // Serial.print("Variable:");
  // Serial.println(500);

  readTemperature();
  simulateConcentration("co2", 0, &co2Time, &co2State);
  simulateConcentration("co", 10000, &coTime, &coState);
  
}
