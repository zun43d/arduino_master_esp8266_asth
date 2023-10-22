#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
 #include "PMS.h"
#include <MQUnifiedsensor.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>

SoftwareSerial nodemcu(2,3); // RX, TX

LiquidCrystal_I2C lcd(0x27,20,4);

PMS pms(Serial);
PMS::DATA dataPMS;

Adafruit_BMP085 bmp;

const int screen_time = 5000;
#define sampling_rate_each_screen 500

float data;
String sen_vals;

// Board setup for MQUnifiedsensor lib
#define board "Arduino UNO"
#define Voltage_Resolution 5
#define ADC_Bit_Resolution 10

// MQ135 init
#define type_135 "MQ-135"
#define pin_135 A0
#define R_air_135 3.6

// MQ7 init
#define type_7 "MQ-7"
#define pin_7 A1
#define R_air_7 27.5

// MQ6 init
#define type_6 "MQ-6"
#define pin_6 A2
#define R_air_6 10

//Declare Sensor
MQUnifiedsensor MQ135(board, Voltage_Resolution, ADC_Bit_Resolution, pin_135, type_135);
MQUnifiedsensor MQ7(board, Voltage_Resolution, ADC_Bit_Resolution, pin_7, type_7);
MQUnifiedsensor MQ6(board, Voltage_Resolution, ADC_Bit_Resolution, pin_6, type_6);

// Sound setup
#define sound_pin A3

// DHT11
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  nodemcu.begin(9600);

  pms.passiveMode();    // Switch to passive mode
  pms.wakeUp();

  dht.begin();
  bmp.begin();

  pinMode(sound_pin, INPUT);

  // MQ Sensor regression set _PPM =  a*ratio^b
  MQ135.setRegressionMethod(1);
  MQ7.setRegressionMethod(1);
  MQ6.setRegressionMethod(1);

  /*
  ########## MQ135 ##########
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */

  MQ135.setA(110.47);
  MQ135.setB(-2.862);  // CO2

  /*
  ########## MQ7 ##########
    Exponential regression:
  GAS     | a      | b
  H2      | 69.014  | -1.374
  LPG     | 700000000 | -7.703
  CH4     | 60000000000000 | -10.54
  CO      | 99.042 | -1.518
  Alcohol | 40000000000000000 | -12.35
  */

  // Notice - MQ7 needs a low/high temperature cycle, but that was not done in this code
  MQ7.setA(99.042);
  MQ7.setB(-1.518);  // CO

  /*
  ########## MQ6 ##########
    Exponential regression:
  GAS     | a      | b
  H2      | 88158  | -3.597
  LPG     | 1009.2 | -2.35
  CH4     | 2127.2 | -2.526
  CO      | 1000000000000000 | -13.5
  Alcohol | 50000000 | -6.017
  */

  MQ6.setA(2127.2);
  MQ6.setB(-2.526);  // CH4

  /*****************************  MQ Init ********************************************/
  MQ135.init();
  MQ7.init();
  MQ6.init();

  // MQ RL
  MQ135.setRL(20);  // From datsheet
  MQ7.setRL(10);    // From datsheet
  MQ6.setRL(20);    // From datsheet  

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print(F("Calibrating"));
  delay(300);

  // MQ calibrations
  float calcR0_135 = 0;
  float calcR0_7 = 0;
  float calcR0_6 = 0;
  for (int i = 1; i <= 11; i++) {
    MQ135.update();  // Update data, the arduino will read the voltage from the analog pin
    MQ7.update();    // Update data, the arduino will read the voltage from the analog pin
    MQ6.update();    // Update data, the arduino will read the voltage from the analog pin

    calcR0_135 += MQ135.calibrate(R_air_135);
    calcR0_7 += MQ7.calibrate(R_air_7);
    calcR0_6 += MQ6.calibrate(R_air_6);

    lcd.setCursor(i + 3, 1);
    lcd.print(F("."));
    delay(300);
  }
  MQ135.setR0(calcR0_135 / 11);
  MQ7.setR0(calcR0_7 / 11);
  MQ6.setR0(calcR0_6 / 11);
}

void loop() {
  unsigned long startTime = millis();

  // MQ135 - CO2
  while (millis() - startTime < screen_time) {
    data = getMq135();

    /*
      CO2 Limit
        Safe 0-350
        Moderate 351 - 450
        Harmful 451 - 1000
        hazardous 1001 - inf
    */

    printGasData("CO2", data, 350, 450, 1000);
    // nodemcu.print(F("mq135"));
    // nodemcu.println(data);
    delay(sampling_rate_each_screen);
  }
  sen_vals += String(data)+",";
 
  startTime = millis();
  // MQ6 - CH4
  while (millis() - startTime < screen_time) {
    data = getMq6();
    
    /*
      Methane Limit
        Safe 0 - 1.8
        Moderate 1.9 - 10
        Harmful 11 - 10000
        Hazardous 10000 - inf
    */

    printGasData("Methane", data, 1.8, 10, 10000);
    // nodemcu.print(F("mq6_v"));
    // nodemcu.println(data);
    delay(sampling_rate_each_screen);
  }
  sen_vals += String(data)+",";
 
  startTime = millis();
  // MQ7 - CO
  while (millis() - startTime < screen_time) {
    data = getMq7();

    /*
      CO Limit
        Safe 0 - 8
        Moderate 9 - 35
        Harmful 36 - 99
        Hazardous 100 - inf
    */

    printGasData("CO", data, 8, 35, 100);
    // nodemcu.print(F("mq7_v"));
    // nodemcu.println(data);
    delay(sampling_rate_each_screen);
  }
  sen_vals += String(data)+",";

  // Sound data
  startTime = millis();
  while (millis() - startTime < screen_time) {
    data = getSound();
    
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("Sound sensor"));

    lcd.setCursor(0, 1);
    lcd.print(F("Loudness: "));
    lcd.print(data);
    lcd.print(F(" dB"));

    if (data <= 50) {
      lcd.setCursor(0, 2);
      lcd.print(F("Level : Quite"));
    } else if (data > 50 && data < 75) {
      lcd.setCursor(0, 2);
      lcd.print(F("Level : Moderate"));
    } else if (data >= 75) {
      lcd.setCursor(0, 2);
      lcd.print(F("Level : High"));
    }

    // nodemcu.print(F("s_val"));
    // nodemcu.println(data);
    delay(sampling_rate_each_screen);
  }
  sen_vals += String(data)+",";

  // Weather data
  startTime = millis();
  float pressure, temp, alti, humid;
  while (millis() - startTime < screen_time) {
    float pressure_Pa = bmp.readPressure();
    pressure = pressure_Pa / 101325;
    temp = bmp.readTemperature();
    alti = bmp.readAltitude();
    // humid = dht.readHumidity();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(F("Temperature: "));
    lcd.print(temp);
    lcd.print(F(" C"));

    lcd.setCursor(0, 1);
    lcd.print(F("Humidity: "));
    // lcd.print(humid);
    lcd.print(F(" %"));

    lcd.setCursor(0, 2);
    lcd.print(F("Altitude: "));
    lcd.print(alti);
    lcd.print(F(" m"));

    lcd.setCursor(0, 3);
    lcd.print(F("Pressure: "));
    lcd.print(pressure);
    lcd.print(F(" atm"));

    
    // nodemcu.print(F("tempc"));
    // nodemcu.println(temp);
    // delay(sampling_rate_each_screen/2);
    // nodemcu.print(F("altit"));
    // nodemcu.println(alti);
    // delay(sampling_rate_each_screen/2);
    // nodemcu.print(F("press"));
    // nodemcu.println(pressure);
    // nodemcu.print(F("humid"));
    // nodemcu.println(humid);
    delay(sampling_rate_each_screen);
  }
  sen_vals += String(temp)+","+String(pressure)+","+String(alti)+","+String(humid)+",";

  // PMS-data
  startTime = millis();
  float pm1, pm25, pm100;
  while (millis() - startTime < screen_time) {
    pms.requestRead();
    if(pms.readUntil(dataPMS)) {
      pm1 = dataPMS.PM_AE_UG_1_0;
      pm25 = dataPMS.PM_AE_UG_2_5;
      pm100 = dataPMS.PM_AE_UG_10_0;
    }
    
    // if (pms.readUntil(dataPMS)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Dust Concentration"));
      lcd.setCursor(0, 1);
      lcd.print("PM1.0 :" + String(pm1) + "(ug/m3)");
      lcd.setCursor(0, 2);
      lcd.print("PM2.5 :" + String(pm25) + "(ug/m3)");
      lcd.setCursor(0, 3);
      lcd.print("PM10  :" + String(pm100) + "(ug/m3)");
    // } else {
    //   lcd.clear();
    //   lcd.setCursor(0, 0);
    //   lcd.print(F("Dust Concentration"));
    //   lcd.setCursor(0, 1);
    //   lcd.print(F("PMS5003 sent no data!"));
    // }
    
    // nodemcu.print(F("pm1_0"));
    // nodemcu.println(pm1);
    // delay(sampling_rate_each_screen/2);
    // nodemcu.print(F("pm2_5"));
    // nodemcu.println(pm25);
    // delay(sampling_rate_each_screen/2);
    // nodemcu.print(F("pm10_"));
    // nodemcu.println(pm100);

    delay(sampling_rate_each_screen);
  }

  sen_vals += String(pm1)+","+String(pm25)+","+String(pm100);
  nodemcu.println(sen_vals);
  Serial.println(sen_vals);
  sen_vals = "";
}

int getSound() {
  const int soundSampleTime = 50;
  unsigned int sample;
  int db;
  
  unsigned long startMillis = millis();  // Start of sample window
  float peakToPeak = 0;                  // peak-to-peak level
  unsigned int signalMax = 0;            //minimum value
  unsigned int signalMin = 1024;         //maximum value

  // collect data for 50 mS
  while (millis() - startMillis < soundSampleTime) {
    sample = analogRead(sound_pin);  //get reading from microphone
    if (sample < 1024)               // toss out spurious readings
    {
      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels
      }
    }
  }

  peakToPeak = signalMax - signalMin;       // max - min = peak-peak amplitude
  db = map(peakToPeak, 20, 900, 49.5, 90);  //calibrate for deciBels

  return db;
}

float getMq135() {
  MQ135.update();
  return MQ135.readSensor();
}

float getMq7() {
  MQ7.update();
  return MQ7.readSensor();
}

float getMq6() {
  MQ6.update();
  return MQ6.readSensor();
}

void printGasData(String gasName, float gasPPM, int safe, int moderate, int harmful) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Gas Sensors"));

    lcd.setCursor(0, 1);
    lcd.print(gasName + " ");
    lcd.print(gasPPM);
    lcd.print(F(" ppm"));

    lcd.setCursor(0, 2);
    lcd.print(F("Level: "));

    if (gasPPM >= 0 && gasPPM <= safe) {
      lcd.print(F("Safe"));
    } else if (gasPPM > safe && gasPPM <= moderate) {
      lcd.print(F("Moderate"));
    } else if (gasPPM > moderate && gasPPM <= harmful) {
      lcd.print(F("Harmful"));
    } else {
      lcd.print(F("Hazardous"));
    }
}