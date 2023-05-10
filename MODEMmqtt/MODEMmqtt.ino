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

const char client_id[]   = "IVR4oiZ62ymacQLSb6qo"; // aca se debe poner el id del device de tdata

int sensorValue = 0;
float currentSum = 0;
int currentCount = 0;
float temperature = 0;
float voltage = 0;
unsigned long myTime;
unsigned long currentTime;
char str[7];

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
  ConnectMQTTClient(client_id);
  myTime = millis();
}

void loop() 
{
  currentTime = millis();
  if(currentTime - myTime <= 15000) {
    sensorValue = analogRead(AI0);
    currentSum += (sensorValue/40.0);
    if(sensorValue/40.0 < 18.0 || sensorValue/40.0 > 21.0)
      SerialMon.println(currentSum/currentCount);
    currentCount++;
  }
  if(currentTime - myTime > 15000) {
    temperature = (float(currentSum/currentCount)*25/100)-60;
    SerialMon.println("\nCurrent Mean: ");
    SerialMon.println(currentSum/currentCount);
    SerialMon.println("\nCurrent Count: ");
    SerialMon.println(currentCount);
    dtostrf(temperature, 4, 2, str);
    PublishData("Temperatura",str);
    currentCount = 0;
    currentSum = 0;
    myTime = millis();
  }
  
  //sensorValue = analogRead(VCCSENSE);
  //voltage = (float(sensorValue)*(25*11)/10000);
  //dtostrf(temperature, 4, 2, str);
  //PublishData("Temperatura",str);
  //dtostrf(voltage, 4, 2, str);
  //PublishData("Tension",str);
}