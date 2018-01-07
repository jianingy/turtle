#include <M5Stack.h>
#include "secrets.h"

const int PIN_EA = 1;
const int PIN_I1 = 16;
const int PIN_I2 = 17;
const int CHANNEL_EA = 3;

const int PIN_EB = 3;
const int PIN_I3 = 21;
const int PIN_I4 = 22;
const int CHANNEL_EB = 4;

bool direction = true;
int current_speed = 0;
int current_duration = 0;
const char delimiters[] = " \r\n\t";

WiFiServer server(80);

void setup() {
  delay(2000);
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.println("staring system ...");
  Serial.begin(9600);
  motor_init();
  connect_wifi();
  M5.Lcd.println("starting control server ...");
  server.begin();
}

void connect_wifi() {
  M5.Lcd.printf("connecting to WiFi [%s] ", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_secret);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  M5.Lcd.println("connected.");
  M5.Lcd.print("IP address is ");
  M5.Lcd.println(WiFi.localIP());
}

void motor_init() {
  M5.Lcd.println("initializing motors ...");

  ledcSetup(CHANNEL_EA, 5000, 8);
  ledcAttachPin(PIN_EA, CHANNEL_EA);
  ledcSetup(CHANNEL_EB, 5000, 8);
  ledcAttachPin(PIN_EB, CHANNEL_EB);

  pinMode(PIN_EA, OUTPUT);
  pinMode(PIN_EB, OUTPUT);
  pinMode(PIN_I1, OUTPUT);
  pinMode(PIN_I2, OUTPUT);
  pinMode(PIN_I3, OUTPUT);
  pinMode(PIN_I4, OUTPUT);

  digitalWrite(PIN_I1, HIGH);
  digitalWrite(PIN_I2, HIGH);
  digitalWrite(PIN_I3, HIGH);
  digitalWrite(PIN_I4, HIGH);

  ledcWrite(CHANNEL_EA, 0);
  ledcWrite(CHANNEL_EB, 0);
}

void activate_right(bool forward, int speed) {
  if (forward) {
    digitalWrite(PIN_I1, LOW);
    digitalWrite(PIN_I2, HIGH);
  } else {
    digitalWrite(PIN_I1, HIGH);
    digitalWrite(PIN_I2, LOW);
  }
  current_speed = speed;
  ledcWrite(CHANNEL_EA, speed);
}

void activate_left(bool forward, int speed) {
  if (forward) {
    digitalWrite(PIN_I3, LOW);
    digitalWrite(PIN_I4, HIGH);
  } else {
    digitalWrite(PIN_I3, HIGH);
    digitalWrite(PIN_I4, LOW);
  }
  current_speed = speed;
  ledcWrite(CHANNEL_EB, speed);
}

void deactivate_right() {
  for (int i = current_speed; i > -1; i--) {
    ledcWrite(CHANNEL_EA, i);
  }
  current_speed = 0;
  digitalWrite(PIN_I1, HIGH);
  digitalWrite(PIN_I2, HIGH);
}

void deactivate_left() {
  for (int i = current_speed; i > -1; i--) {
    ledcWrite(CHANNEL_EB, i);
  }
  current_speed = 0;
  digitalWrite(PIN_I3, HIGH);
  digitalWrite(PIN_I4, HIGH);
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    while (client.connected()) {
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);

      if (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        M5.Lcd.printf("command received [%s]\n", line.c_str());
        char *verb = strtok((char *)line.c_str(), delimiters);
        M5.Lcd.printf("verb = %s, ", verb);
        if (strcmp(verb, "FD") == 0 || strcmp(verb, "RD") == 0
            || strcmp(verb, "LT") == 0 || strcmp(verb, "RT") == 0) {
          const int speed = atoi(strtok(NULL, delimiters));
          const int duration = atoi(strtok(NULL, delimiters));
          M5.Lcd.printf("speed = %04d, duration = %04d\n", speed, duration);
          if (speed > 0 && duration > 0) {
            if (strcmp(verb,"FD") == 0) {
              activate_left(true, speed);
              activate_right(true, speed);
            } else if (strcmp(verb, "RD") == 0) {
              activate_left(false, speed);
              activate_right(false, speed);
            } else if (strcmp(verb, "LT") == 0) {
              activate_left(false, speed);
              activate_right(true, speed);
            } else if (strcmp(verb, "RT") == 0) {
              activate_left(true, speed);
              activate_right(false, speed);
            }
            current_duration = duration;
            delay(duration);
          }
        } else {
          M5.Lcd.printf("error = invalid verb\n");
          break;
        }
        deactivate_left();
        deactivate_right();
      }
    }
    delay(1);
    client.stop();
  }
  M5.update();
}
