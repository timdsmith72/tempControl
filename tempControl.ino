#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Wire.h>
#include <EEPROM.h>
#include <TrueRandom.h>
#include <avr/wdt.h>
#define FREEZER 5 //This is the freezer arduino pin.
#define HEATER 6  // Heater arduino pin.
#define RED 0x1  //These are just setting up the LCD
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define COMPRESSOR_STARTUP_DELAY 180
//#define COMPRESSOR_STARTUP_DELAY 20
//  number of seconds between readings.
#define SLEEP_SECONDS 10


byte mac[6] = { 0x90, 0xA2, 0xDA, 0x00, 0x00, 0x00 };
char macstr[18];
byte ip[] = {192,168,1,30}; //Arduino IP

byte TEMP_SENSOR = 3; // Dallas Ds18B20 Temperature Sensor is on Pin 3
 
double targetTemp; //Temperature we're trying to hold.
double maxTemp;  //Maximum temp we'll let it get to.
double minTemp;  //Minimum temp we'll let it get to.

OneWire oneWire(TEMP_SENSOR); // Conntected to digital pin 3

const int numReadings = 6;
double readings[numReadings]; // the readings from the analog input

int index = 0; // the index of the current reading
double total = 0; // the running total
double average = 0; // the average
float tempF; //Temperature in F
float tempC; //Temperature in C.  The OneWire sensors read in degrees C by default.
float temperature; // Just initialize the temperature variable.  It needs to be global cause it's used all over the place.
int stringPos = 0;  
char inString[32];
boolean startRead = false;
int start_compressor = COMPRESSOR_STARTUP_DELAY;
uint8_t compressor = HIGH;  
uint8_t heater = HIGH;
int delta;
unsigned long last_checked = 0;
unsigned long this_check = 0;
time_t lastSentDataTime;
const unsigned long sendDataInterval = 30;
boolean freezerON;
boolean heaterON;


Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Initialize the Ethernet client library
// with the IP address and port of the server 
EthernetClient client;
byte serverName[] = {192, 168, 1, 3}; // Database server IP

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);  // Setting it up this way so I can add more sensors later if I want.
//Hard code the device address so it doesn't have to search for it each time.
//That just seems like a waste of cycles to me....
DeviceAddress beerSensor = { 0x28, 0x18, 0xB6, 0xBF, 0x04, 0x00, 0x00, 0xA2 };

void setup() {
  wdt_disable();
  wdt_enable(WDTO_8S); // This whole routine shouldn't take more than 8 seconds.  If it does, it's probably hung.  Reset it.
  wdt_reset();
  Serial.begin(9600);
  delay(1000);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);  
  setupEthernet();
   
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.setBacklight(GREEN);
  pinMode(FREEZER, OUTPUT);
  pinMode(HEATER, OUTPUT);
  digitalWrite(FREEZER, HIGH);  //  High is off for this relay.  Here, we're making sure that the freezer isn't turned on as soon as you plug in the Arduino.
  digitalWrite(HEATER, HIGH);  //  High is off for this relay.  Here, we're making sure that the heater isn't turned on as soon as you plug in the Arduino.
  targetTemp = getTargetTemp();  //getTargetTemp() goes and grabs the Target Temp out of the database.
  if (targetTemp < 20){  //  When it can't get a temp, it comes back as zero.  So let's check for that.
    Serial.println("Couldn't get target temp.  Trying again.");
    delay(1000);
    setupEthernet();  //  Try setting up ethernet again just in case.
    targetTemp = getTargetTemp();
    if (targetTemp < 20){
      Serial.println("Failed twice getting target Temp.  Resetting.");  //  Sometimes it just loses ability to connect.  Reset fixes that.  
      completeReset();  //  Also, if the arduino reboots for some reason.  it's never able to reconnect until a reset.
    }  //  Another also.  Sometimes it thinks it got a target, but it really got ZERO.  We don't want to kill our yeast, so we should reset if this happens.
    else{
      Serial.println("Got target Temp.  Moving on.");
    }
  }
  else{
    Serial.println("2 Got target Temp.  Moving on.");
  }

  maxTemp = targetTemp + 1;  //  I want to keep it within 1 degree of setpoint.  You can adjust as necessary.
  minTemp = targetTemp - 1;
  freezerON = false;  
  heaterON = false;
  sensors.begin();  // initialize the temp sensor.
  // set the resolution to 10 bit (good enough I think?)
  sensors.setResolution(beerSensor, 10);
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0; 
}

float getTemp(DeviceAddress beerSensor){
  
  //returns the temperature from one DS18S20 in DEG Celsius
  Serial.print("Getting temperatures...\n\r");
  tempC = sensors.getTempC(beerSensor);
  if (tempC == -127.00) {
    Serial.print("Error getting temperature\n\r");
  } else {
    //  Convert the reading to DEG F.  C just hurts my noggin too much.  
    tempF = DallasTemperature::toFahrenheit(tempC);
    return tempF;
  }
}

void lcdPrint(float temperature, float setPoint){
  lcd.setCursor(0, 0);  //Set cursor position to first character of first line.
  lcd.print("Target:  ");
  lcd.print(setPoint);
  lcd.setCursor(0, 1);  //Set to first character of second line.
  lcd.print("Temp:  ");
  lcd.print(temperature);
  if(freezerON){
     lcd.setBacklight(BLUE); 
  }
  if(heaterON){
    lcd.setBacklight(RED);
  }
  if(!freezerON && !heaterON){
    lcd.setBacklight(GREEN);
  }
}

void lcdPrintWaiting(){ // I like a visual indication of whether it's waiting the 3 minutes to turn the freezer back on.
   lcd.setCursor(15, 1); // So I just print an asterisk in the last position on the second line.
   lcd.print("*"); 
}

void lcdPrintNotWaiting(){  //  Or not.  :)
   lcd.setCursor(15, 1);
   lcd.print(" "); 
}

void sendData(float temperature ){
    Serial.println("connecting...");
    //if you get a connection, report back via serial:
    if (client.connect(serverName, 80)) {
    Serial.println("connected");
    // Make a HTTP request:  Sends information to "data.php" which lives on the RPi.
    client.print("GET http://192.168.1.3/tempControl/data.php?Temperature="); //place your server address here
    Serial.print("Sent Temp:  ");  //  
    Serial.println(temperature);
    client.print(temperature);  // 
    client.print("&Setpoint=");
    client.print(targetTemp);
    client.println(" HTTP/1.0");
    client.println("Host: http://192.168.1.3");
    client.println();
    client.stop();
    delay(300);
    } else {
      Serial.println("connection failed in SendData()");
      client.stop();
      delay(300);
      Ethernet.begin(mac, ip);  //  Start ethernet again and we'll try it again next time through.
    }
    lastSentDataTime = now();
}

String convertTargetTemp(){
    //read the page returned py the PHP script we called
    // and capture & everything between '<' and '>' 
    stringPos = 0;
    memset( &inString, 0, 32 ); //clear inString memory
 
    while (true)
    {
 
        if (client.available())
        {
            while (client.connected())
            {
                char c = client.read();
                //Serial.println(c);
                // How long to wait for it all to arrive?
                if (c == '<' )   //'<' is our begining character
                {
                    startRead = true; //Ready to start reading the part
                }
                else if (startRead)
                {
 
                    if (c != '>') //'>' is our ending character
                    {
                        inString[stringPos] = c;
                        stringPos ++;
                    }
                    else
                    {
                        //got what we need here! We can disconnect now
                        startRead = false;
                        while(client.connected()){
                          client.stop();
                          client.flush();
                          delay(1000);
                        }
                        //Serial.println(inString);
                        return inString;  // Send everything we got back to the calling function.
                    }
                }
            }
        }
    }
}

double getTargetTemp()
{
    if (client.connect(serverName, 80))
    {
      Serial.println("Connected to Server in getTargetTemp()");
      delay(1000);
      // format and  send the HTTP GET request: Makes call to "getSetTemp.php" on the RPi.
      client.print("GET http://192.168.1.3/tempControl/getSetTemp.php?");
      client.println(" HTTP/1.1"); 
      client.println("Host: localhost");          //run the php script on the server
      client.println("Accept: text/html");        //
      client.println("Connection: close");
      client.println();
      convertTargetTemp();
      client.stop();
      delay(500);
      return atof(inString);
    } else {
      Serial.println("connection failed in GetTargetTemp()");
      client.stop();
      delay(500);
      setupEthernet();
    }
}

void setupEthernet(){
  //  This generates a random mac address for your Ethernet card to use.
  //  They use to come with a MAC printed on them.  I guess they don't any more.  I just generate one.  
  if (EEPROM.read(1) == '#') {
    for (int i = 3; i < 6; i++) {
      mac[i] = EEPROM.read(i);
    }
  } else {
    for (int i = 3; i < 6; i++) {
      mac[i] = TrueRandom.randomByte();
      EEPROM.write(i, mac[i]);
    }
    EEPROM.write(1, '#');
  }
  snprintf(macstr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Ethernet.begin(mac,ip);
  Serial.println("Starting Ethernet.");
  delay(1000);
}

void completeReset(void){
  // This will perform a complete reset of the arduino device.  
  // I put this in just in case it loses its ability to connect to the 
  // Raspberry Pi.  
  Serial.println("In CompleteReset().  Waiting 8 seconds.");
  wdt_enable(WDTO_8S);
  while(1){
  }
}

void loop(void) {
  wdt_reset();  //  Reset the WDT to zero.  
  this_check = millis();
  if(client.connected()){
    client.stop();
  }
  //Serial.println(this_check);
  //Serial.println(last_checked);
  //Serial.println(SLEEP_SECONDS * 1000);
  if(this_check > last_checked + SLEEP_SECONDS * 1000)
  {
    delta = int((this_check - last_checked) / 1000);
    if(start_compressor > 0) start_compressor -= delta; // Check to see if it's been at least 3 minutes since the Freezer was last on.
    sensors.requestTemperatures();
    temperature = getTemp(beerSensor);
    lcdPrint(temperature, targetTemp);
    readings[index] = temperature;
    total= total + readings[index];
    index = index + 1; 
    // calculate the average:
    average = total / index; 
    Serial.print("Setpoint:  ");
    Serial.println(targetTemp);
    //Serial.print("  Temperature:  ");
    //Serial.println(temperature);
    time_t rightNow = now();   
    Serial.println(rightNow - lastSentDataTime); 
    if(rightNow - lastSentDataTime > sendDataInterval){  
      sendData(average);
      targetTemp = getTargetTemp();
      if (targetTemp < 20){
        Serial.println("Couldn't get target temp.  Trying again.");
        setupEthernet(); // Try setting up the ethernet again, just to see.
        delay(1000);
        targetTemp = getTargetTemp();
        if (targetTemp < 20){
          Serial.println("Failed twice getting target Temp.  Resetting.");
          completeReset();
        }
        else{
          Serial.println("Got target Temp.  Moving on.");
        }
      }
      else{
        Serial.println("2 Got target Temp.  Moving on.");
      }
       
      maxTemp = targetTemp + 1;
      minTemp = targetTemp - 1;
      index = 0;
      total = 0;
    }
    
    //  Here's where we will control the FREEZER and HEATER
    Serial.print("Start Compressor");
    Serial.println(start_compressor);
    if(start_compressor < 1)
    {
     //68              //69
      if(!freezerON)
      {
          if(temperature > maxTemp){  // Turn it on.
            compressor = LOW; //HIGH for LED //LOW is ON for my relay.
            freezerON = true;
          }
          else{  //Leave it off.
            compressor = HIGH; //LOW is OFF for LED  //HIGH Is off for relay
            freezerON = false;
          }    
      }
      else{  //  Freezer is ON
        if(temperature <= targetTemp){  //Turn it off.
          compressor = HIGH; //LOW is off for LED.  //HIGH is OFF for relay
          freezerON = false;
          start_compressor = COMPRESSOR_STARTUP_DELAY;  //Reset the startup delay
        }else{  //Leave it on.  
           compressor = LOW;  //HIGH on for LED  // LOW is on for relay.  Leave it on. 
           freezerON = true;
        }
      }
      digitalWrite(FREEZER, compressor);
      //}
      if(!heaterON){
        if(temperature < minTemp){
          heater = LOW; // Turn Heater on
          heaterON = true;
        }else{
          heater = HIGH; // Leave it off
          heaterON = false;
        }    
      }
      else{
        if(temperature >= targetTemp){
          heater = HIGH;  // Turn Heater off
          heaterON = false;
        }else{
          heater = LOW;  // Leave it on
          heaterON = true;
        }
      }
      digitalWrite(HEATER, heater);
      lcdPrintNotWaiting();
    }else{
      Serial.println("Waiting....");
       lcdPrintWaiting();
    }
    last_checked = this_check;
  }
  wdt_reset();
}


