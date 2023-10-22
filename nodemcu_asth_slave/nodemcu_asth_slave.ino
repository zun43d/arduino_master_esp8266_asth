
#define BLYNK_TEMPLATE_ID "TMPL6WmGAEtYm"
#define BLYNK_TEMPLATE_NAME "Environment Monitoring System"
#define BLYNK_AUTH_TOKEN "X-RskJ2SKpLo1Eg6Lo-cNPoaoSn64vJ9"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define V_0    V0 
#define V_1    V1
#define V_2    V2
#define V_3    V3
#define V_4    V4
#define V_5    V5
#define V_6    V6
#define V_7    V7
#define V_8    V8
#define V_9    V9

char ssid[] = "My Deco";
char pass[] = "Alvi77685575";

bool connectedFlag = false;
float sen_vals[11]; // mq135,mq6,mq7,sound,temp,press,alti,humid,pm1,pm25,pm100

// BlynkTimer timer;

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // timer.setInterval(1000L, updateState);
}

void loop() {
  if (Serial.available() > 0 ) {
    String data = Serial.readString();
    // String sensor = data.substring(0, 5);
    // float sen_val = data.substring(5).toFloat();

    int count = splitString(data, sen_vals);

    for(int i=0; i<count; i++)
      Serial.print(sen_vals[i]);
    Serial.println();

    connectedFlag = true;
    updateState();
  }
  if(connectedFlag) Blynk.run();
  // timer.run();
}

void updateState() {
  Blynk.virtualWrite(V_0, sen_vals[0]); // mq135
  Blynk.virtualWrite(V_1, sen_vals[2]); // mq7
  Blynk.virtualWrite(V_2, sen_vals[1]); // mq6
  Blynk.virtualWrite(V_3, sen_vals[8]); // pm1.0
  Blynk.virtualWrite(V_4, sen_vals[9]); // pm2.5
  Blynk.virtualWrite(V_5, sen_vals[10]);// pm10.0
  Blynk.virtualWrite(V_6, sen_vals[4]);  // temp
  Blynk.virtualWrite(V_7, sen_vals[7]);  // humid
  Blynk.virtualWrite(V_8, sen_vals[5]);  // pressure
  Blynk.virtualWrite(V_9, sen_vals[3]);  // sound
}

int splitString(String input, float arr[]) {
  int valueCount = 0;
  int startIndex = 0;
  int endIndex = input.indexOf(',');

  while (endIndex != -1) {
    arr[valueCount] = input.substring(startIndex, endIndex).toFloat();
    valueCount++;
    startIndex = endIndex + 1;
    endIndex = input.indexOf(',', startIndex);
  }

  // Add the last value (after the last comma)
  if (startIndex < input.length()) {
    arr[valueCount] = input.substring(startIndex).toFloat();
    valueCount++;
  }

  return valueCount;
}