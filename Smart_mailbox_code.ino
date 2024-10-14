#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <HX711.h>
#include <RTClib.h>

SoftwareSerial sim800l(3, 2); // RX, TX for Arduino and for the module it's TXD, RXD; they should be inverted
Servo pls; // package lock servo
Servo cls; // collecting lock servo

int x = 0;
int y = 0;

int state = 0;

const int IR_PIN3 = 4; // Arduino pin connected to the IR sensor
const int LED_PIN = 13;
const int lockButtonPin = 7;
const int plsmagnet = 9;

int index = 0;
String number = "";
String message = "";

char incomingByte;
String incomingData;
bool atCommand = true;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void receivedMessage(String inputString);
void SendSMS();
void clsUnlock();
void clsLock();
void plsUnlock();
void plsLock();


RTC_DS1307 rtc; // Create an instance of the RTC_DS1307 class

void setup() {
  clsLock();

  pls.attach(12);  // Attach the servo to pin 12
  cls.attach(10); // Attach the servo to pin 10

  pinMode(IR_PIN3, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(lockButtonPin, INPUT);
  pinMode(plsmagnet, INPUT);

  if (y < 1) {
    plsUnlock();
  } else {
    plsLock();
  }

  sim800l.begin(9600);
  Serial.begin(9600);
  delay(1000);

  // Check if you're currently connected to SIM800L
  while (!sim800l.available()) {
    sim800l.println("AT");
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected.");

  sim800l.println("AT+CMGF=1");  // Set SMS Text Mode
  delay(1000);
  sim800l.println("AT+CNMI=1,2,0,0,0");  // Procedure: how to receive messages from the network
  delay(1000);
  sim800l.println("AT+CMGL=\"REC UNREAD\""); // Read unread messages

  Serial.println("Ready to receive commands.");

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Letters");
  lcd.setCursor(0, 1);
  lcd.print(x);
  lcd.print(" ");

  if (!rtc.begin()) {
    while (1);
  }
}

void loop() {
  DateTime now = rtc.now();

  if (digitalRead(IR_PIN3) == LOW) {
    digitalWrite(LED_PIN, HIGH);
    SendSMS();
    delay(500);
  } else {
    Serial.println(".");
    digitalWrite(LED_PIN, LOW);
  }

  int counter = digitalRead(LED_PIN);

  if (state == 0) {
    switch (counter) {
      case 1:
        state = 1;
        lcd.setCursor(0, 1);
        x = x + 1;
        lcd.print(x);
        break;
      case 0:
        state = 0;
        break;
    }
  }

  if (counter == 0) {
    state = 0;
  }

  if (digitalRead(plsmagnet) == HIGH) {
    delay(1000);

    plsLock();
  }

  if (digitalRead(lockButtonPin) == LOW) {
    delay(100); // Simple debounce delay
    if (digitalRead(lockButtonPin) == HIGH) {
      clsLock();
    }
  }


  if (sim800l.available()) {
    delay(100);

    while (sim800l.available()) {
      incomingByte = sim800l.read();
      incomingData += incomingByte;
    }

    delay(10);
    if (atCommand == false) {
      receivedMessage(incomingData);
    } else {
      atCommand = false;
    }


    if (incomingData.indexOf("OK") == -1) {
      sim800l.println("AT+CMGDA=\"DEL ALL\"");
      delay(1000);
      atCommand = true;
    }

    incomingData = "";
  }




}

void receivedMessage(String inputString) {
  //Get The number of the sender
  index = inputString.indexOf('"') + 1;
  inputString = inputString.substring(index);
  index = inputString.indexOf('"');
  number = inputString.substring(0, index);
  Serial.println("Number: " + number);

  //Get The Message of the sender
  index = inputString.indexOf("\n") + 1;
  message = inputString.substring(index);
  message.trim();
  Serial.println("Message: " + message);
  message.toUpperCase(); // uppercase the message received

  //verifing the number

  if (number.indexOf("+94769205348") > -1) {    // update number here
    if (message.indexOf("UNLOCK") > -1) {
      clsUnlock();
      delay(1000);
      plsUnlock();
      delay(1000);
      Serial.println("door unlocked");
    }
  }
}

void SendSMS() {
  Serial.println("Sending SMS..."); // Show this message on the serial monitor
  sim800l.print("AT+CMGF=1\r"); // Set the module to SMS mode
  delay(100);
  sim800l.print("AT+CMGS=\"+94769205348\"\r"); // Send SMS to authorized phone number
  delay(500);
  sim800l.print("You have received a letter\nNumber of letters inside the box: ");
  sim800l.print(x + 1); // This is the text to send to the phone number
  delay(500);
  sim800l.print((char)26); // (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}

void clsUnlock() {
  for (int i = 0; i <= 180; i += 1) {
    cls.write(i);
    delay(10);

  }
  delay(1000);

  Serial.println("Sending SMS..."); // Show this message on the serial monitor
  sim800l.print("AT+CMGF=1\r"); // Set the module to SMS mode
  delay(1000);
  sim800l.print("AT+CMGS=\"+94769205348\"\r"); // Send SMS to authorized phone number
  delay(500);
  sim800l.print("Access granted: Mail collection lock unlocked.\nTotal Letters : ");
  sim800l.print(x);

  delay(500);
  x = 0;
  lcd.setCursor(0, 1);
  lcd.print("        "); // Clear the previous counter value
  lcd.setCursor(0, 1);
  lcd.print(x);
  sim800l.print((char)26); // (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}

void clsLock() {
  for (int i = 180; i >= 0; i -= 1) {
    cls.write(i);
    delay(10);
  }
  delay(1000);

  Serial.println("Sending SMS..."); // Show this message on the serial monitor
  sim800l.print("AT+CMGF=1\r"); // Set the module to SMS mode
  delay(1000);
  sim800l.print("AT+CMGS=\"+94769205348\"\r"); // Send SMS to authorized phone number
  delay(500);
  sim800l.print("Mailbox locked and secured.");
  sim800l.print((char)26); // (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);

}

void plsUnlock() {
  for (int i = 0; i <= 180; i += 1) {
    pls.write(i);
    delay(10);
  }
  delay(1000);
  y = 0;
  lcd.setCursor(0, 1);
  lcd.print("        "); // Clear the previous counter value
  lcd.setCursor(0, 1);
  lcd.print(y);
  sim800l.print((char)26); // (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}

void plsLock() {
  for (int i = 180; i >= 0; i -= 1) {
    pls.write(i);
    delay(10);
  }
  delay(1000);
  Serial.println("Sending SMS..."); // Show this message on the serial monitor
  sim800l.print("AT+CMGF=1\r"); // Set the module to SMS mode
  delay(100);
  sim800l.print("AT+CMGS=\"+94769205348\"\r"); // Send SMS to authorized phone number
  delay(500);
  sim800l.print("You have received a package, please collect it");
  sim800l.print((char)26); // (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}
