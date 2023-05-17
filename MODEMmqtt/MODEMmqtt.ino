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

int sensorValue = 0;
float meanC = 0;
float minC = 20;
float maxC = 4;
int currentCount = 0;
float temperature = 0;
float minTemperature = 0;
float maxTemperature = 0;
float voltage = 0;
unsigned long startTime;
unsigned long currentTime;
char str[7];

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
}

void loop() 
{
  delay(5000);

  currentTime = millis();
  sensorValue = analogRead(AI0)/40;
  updateMin(sensorValue);
  updateMax(sensorValue);
  meanC = (meanC * currentCount + sensorValue)/(currentCount + 1);
  currentCount++;

  SerialMon.println("\nCurrent meanC: ");
  SerialMon.println(meanC);

  SerialMon.println("\nCurrent minC: ");
  SerialMon.println(minC);

  SerialMon.println("\nCurrent maxC: ");
  SerialMon.println(maxC);

  if(currentTime - startTime > 30000) {
    temperature = calculateTemperature(meanC);
    SerialMon.println("\nCurrent Mean: ");
    SerialMon.println(temperature);
    
    dtostrf(temperature, 4, 2, str);
    PublishData("Temperatura Promedio",str);

    minTemperature = calculateTemperature(minC);
    
    dtostrf(minTemperature, 4, 2, str);
    PublishData("Temperatura Mínima",str);

    maxTemperature = calculateTemperature(maxC);
    
    dtostrf(maxTemperature, 4, 2, str);
    PublishData("Temperatura Máxima",str);
    
    currentCount = 0;
    startTime = millis();
  }
  
  //sensorValue = analogRead(VCCSENSE);
  //voltage = (float(sensorValue)*(25*11)/10000);
  //dtostrf(temperature, 4, 2, str);
  //PublishData("Temperatura",str);
  //dtostrf(voltage, 4, 2, str);
  //PublishData("Tension",str);
}