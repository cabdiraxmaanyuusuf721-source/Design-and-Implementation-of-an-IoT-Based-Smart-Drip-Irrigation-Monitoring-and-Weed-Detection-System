/************************************************************
   IoT-Based Smart Drip Irrigation Monitoring
   and Weed Detection System

   ESP32 + Blynk + DHT11 + Soil Sensor + Rain Sensor
   + TCS3200 Color Sensor + LCD + Relay Pump

************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL2BpTLQX7H"
#define BLYNK_TEMPLATE_NAME "IoT Based Drip Irrigation and Weed Monitoring"
#define BLYNK_AUTH_TOKEN "3U_Is8ByQL1VZYTLdaB0hngy9_IWMsvh"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//================= WIFI =================
char ssid[] = "Maan";
char pass[] = "ffffffff";

//================= LCD ==================
LiquidCrystal_I2C lcd(0x27, 16, 2);

//================= DHT ==================
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//================= PINS =================
#define RELAY_PIN    12
#define SOIL_PIN     19
#define RAIN_PIN     14
#define GREEN_LED    18
#define RED_LED      5
#define BUZZER       17

//================= TCS3200 =================
#define S0           27
#define S1           25
#define S2           32
#define S3           33
#define SENSOR_OUT   34

//================= VARIABLES =================
BlynkTimer timer;

bool pumpWasOn = false;
bool weedDetected = false;

bool manualPump = false;
bool autoMode = true;

int redValue = 0;
int greenValue = 0;
int blueValue = 0;

//================================================
// BLYNK MANUAL PUMP CONTROL
//================================================
BLYNK_WRITE(V9)
{
  manualPump = param.asInt();

  if (manualPump == 1)
  {
    autoMode = false;
  }
  else
  {
    autoMode = true;
  }
}

//================================================
// BUZZER
//================================================
void beepAlarm()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BUZZER, HIGH);
    delay(250);
    digitalWrite(BUZZER, LOW);
    delay(150);
  }
}

//================================================
// TCS3200 COLOR FUNCTIONS
//================================================
int readRed()
{
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(SENSOR_OUT, LOW);
}

int readGreen()
{
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(SENSOR_OUT, LOW);
}

int readBlue()
{
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(SENSOR_OUT, LOW);
}

//================================================
// MAIN SENSOR FUNCTION
//================================================
void sendData()
{
  //================= READ DHT =================
  float temperature = dht.readTemperature();

  if (isnan(temperature))
    temperature = 0;

  //================= READ SENSORS =================
  int soilValue = digitalRead(SOIL_PIN);
  int rainValue = digitalRead(RAIN_PIN);

  //================= READ RGB =================
  redValue = readRed();
  delay(20);

  greenValue = readGreen();
  delay(20);

  blueValue = readBlue();
  delay(20);

  //================================================
  // WEED DETECTION
  // Smaller frequency value = stronger color
  //================================================
  if ((greenValue < redValue) && (greenValue < blueValue))
  {
    weedDetected = true;
  }
  else
  {
    weedDetected = false;
  }

  //================================================
  // PUMP CONTROL
  //================================================
  bool pumpOn = false;

  if (autoMode)
  {
    // Soil DRY + No Rain
    if (soilValue == HIGH && rainValue == HIGH)
    {
      pumpOn = true;
    }
  }
  else
  {
    pumpOn = manualPump;
  }

  //================================================
  // RELAY CONTROL
  //================================================
  if (pumpOn)
  {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(RED_LED, HIGH);

    if (!pumpWasOn)
    {
      beepAlarm();
    }

    pumpWasOn = true;
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(RED_LED, LOW);
    pumpWasOn = false;
  }

  // System LED
  digitalWrite(GREEN_LED, HIGH);

  //================================================
  // LCD DISPLAY
  //================================================
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(10, 0);

  if (rainValue == LOW)
    lcd.print("RAIN");
  else
    lcd.print("DRY ");

  lcd.setCursor(0, 1);

  if (weedDetected)
  {
    lcd.print("WEED DETECTED");
  }
  else if (pumpOn)
  {
    lcd.print("PUMP ON");
  }
  else
  {
    lcd.print("PUMP OFF");
  }

  //================================================
  // SERIAL MONITOR
  //================================================
  Serial.println("================================");

  Serial.print("Temperature: ");
  Serial.println(temperature);

  Serial.print("Soil: ");
  Serial.println(soilValue == HIGH ? "DRY" : "WET");

  Serial.print("Rain: ");
  Serial.println(rainValue == LOW ? "RAIN" : "NO RAIN");

  Serial.print("Red: ");
  Serial.println(redValue);

  Serial.print("Green: ");
  Serial.println(greenValue);

  Serial.print("Blue: ");
  Serial.println(blueValue);

  Serial.print("Weed: ");
  Serial.println(weedDetected ? "YES" : "NO");

  //================================================
  // STATUS MESSAGE
  //================================================
  String statusMsg;

  if (!autoMode)
  {
    if (pumpOn)
      statusMsg = "Manual Pump ON";
    else
      statusMsg = "Manual Mode";
  }
  else if (rainValue == LOW)
  {
    statusMsg = "Rain Detected";
  }
  else if (weedDetected)
  {
    statusMsg = "Weed Detected";
  }
  else if (pumpOn)
  {
    statusMsg = "Auto Pump ON";
  }
  else
  {
    statusMsg = "System Normal";
  }

  //================================================
  // BLYNK UPDATE
  //================================================
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, soilValue);
  Blynk.virtualWrite(V2, pumpOn);
  Blynk.virtualWrite(V3, weedDetected);
  Blynk.virtualWrite(V4, (rainValue == LOW ? 1 : 0));
  Blynk.virtualWrite(V5, redValue);
  Blynk.virtualWrite(V6, greenValue);
  Blynk.virtualWrite(V7, blueValue);
  Blynk.virtualWrite(V8, statusMsg);

  //================================================
  // BLYNK EVENTS
  //================================================
  static bool weedEventSent = false;

  if (weedDetected && !weedEventSent)
  {
    Blynk.logEvent(
      "weed_detected",
      "Weed detected near irrigation area."
    );

    weedEventSent = true;
  }

  if (!weedDetected)
  {
    weedEventSent = false;
  }
}

//================================================
// SETUP
//================================================
void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // TCS3200
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);

  // Frequency Scaling 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER, LOW);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART DRIP");

  lcd.setCursor(0, 1);
  lcd.print("CONNECTING...");

  // DHT
  dht.begin();

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Timers
  timer.setInterval(2000L, sendData);

  delay(2000);
  lcd.clear();
}

//================================================
// LOOP
//================================================
void loop()
{
  Blynk.run();
  timer.run();
}