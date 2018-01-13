#include <M5Stack.h>
#include <aWOT.h>
#include <dashboard.h>

const int PIN_EA = 1;
const int PIN_I1 = 16;
const int PIN_I2 = 17;
const int CHANNEL_EA = 3;

const int PIN_EB = 3;
const int PIN_I3 = 21;
const int PIN_I4 = 22;
const int CHANNEL_EB = 4;

const int PIN_SRF_TRIGGER = 2;
const int PIN_SRF_ECHO = 5;

int mode = 0;
bool direction = true;
int limited_speed = 255;
int running_speed = 0;
int current_duration = 0;
const char delimiters[] = " \r\n\t";
const char wifi_ssid[] = "TURTLE-MOTOR";

WiFiServer server(80);
WebApp webapp;

void setup() {
  delay(2000);
  M5.begin();
  M5.Lcd.clear();
  M5.Lcd.println("staring system ...");
  Serial.begin(9600);
  motor_init();
  sensor_init();
  ap_init();
  M5.Lcd.println("starting control server ...");
  server.begin();
  webapp_init();
}

void ap_init() {
  M5.Lcd.printf("starting WiFi %s ...\n", wifi_ssid);
  WiFi.softAP(wifi_ssid);
  M5.Lcd.print("IP address is ");
  M5.Lcd.println(WiFi.softAPIP());
}

void sensor_init() {
  M5.Lcd.println("initializing sensors ...");
  pinMode(PIN_SRF_TRIGGER, OUTPUT);
  pinMode(PIN_SRF_ECHO, INPUT);
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

void webapp_init() {
  webapp.get("/", &dashboard_view);
  webapp.post("/do", &command_view);
}

void dashboard_view(Request &req, Response &resp) {
  resp.success("text/html");
  resp.printP(dashboard_html);
}

void command_view(Request &req, Response &resp) {
  char *command = req.query("command");
  int speed = 240;
  if (strcmp(command, "FD") == 0) {
    activate_left(true, speed);
    activate_right(true, speed);
  } else if (strcmp(command, "RD") == 0) {
    activate_left(false, speed);
    activate_right(false, speed);
  } else if (strcmp(command, "LT") == 0) {
    activate_left(false, speed);
    activate_right(true, speed);
  } else if (strcmp(command, "RT") == 0) {
    activate_left(true, speed);
    activate_right(false, speed);
  } else {
    deactivate_left();
    deactivate_right();
  }
  resp.success("text/html");
  resp.printP("OK");
}

int available_distance() {
  int num = 10;
  float sum_distance = 0.0;
  float max_value = -1.0, min_value = 9999.9;
  for (int i = 0; i < num; i++) {
    // Trigger SRF-05
    digitalWrite(PIN_SRF_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_SRF_TRIGGER, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_SRF_TRIGGER, LOW);

    // Read distance
    int duration = pulseIn(PIN_SRF_ECHO, HIGH);
    float distance = (duration / 2.0) / 29.1;
    if (distance > max_value)
      max_value = distance;
    if (distance < min_value)
      min_value = distance;
    sum_distance += distance;
  }

  return int((sum_distance - max_value - min_value) / (num - 2));
}

void activate_right(bool forward, int speed) {
  if (forward) {
    digitalWrite(PIN_I1, LOW);
    digitalWrite(PIN_I2, HIGH);
  } else {
    digitalWrite(PIN_I1, HIGH);
    digitalWrite(PIN_I2, LOW);
  }
  ledcWrite(CHANNEL_EA, speed);
  running_speed = speed;
}

void activate_left(bool forward, int speed) {
  if (forward) {
    digitalWrite(PIN_I3, LOW);
    digitalWrite(PIN_I4, HIGH);
  } else {
    digitalWrite(PIN_I3, HIGH);
    digitalWrite(PIN_I4, LOW);
  }
  ledcWrite(CHANNEL_EB, speed);
  running_speed = speed;
}

void deactivate_right() {
  for (int i = running_speed; i > -1; i--) {
    ledcWrite(CHANNEL_EA, i);
  }
  running_speed = 0;
  digitalWrite(PIN_I1, HIGH);
  digitalWrite(PIN_I2, HIGH);
}

void deactivate_left() {
  for (int i = running_speed; i > -1; i--) {
    ledcWrite(CHANNEL_EB, i);
  }
  running_speed = 0;
  digitalWrite(PIN_I3, HIGH);
  digitalWrite(PIN_I4, HIGH);
}

void command_mode() {
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
}

void auto_mode() {
  int distance = available_distance();
  M5.Lcd.setCursor(M5.Lcd.width() / 2, 100);
  M5.Lcd.printf("%d", limited_speed);
  while (distance < 40) {
    M5.Lcd.fillScreen(RED);
    activate_left(false, limited_speed * 0.8);
    activate_right(true, limited_speed * 0.8);
    distance = available_distance();
    delay(10);
  }
  M5.Lcd.fillScreen(GREEN);
  activate_left(true, limited_speed);
  activate_right(true, limited_speed);
  delay(10);
}

void manual_mode() {
  WiFiClient client = server.available();

  if (client) {
    webapp.process(&client);
  }
}

void loop() {
  if (mode == 0) {
    manual_mode();
  } else if (mode == 1) {
    auto_mode();
  } else if (mode == 3) {
    command_mode();
  }

  if (M5.BtnA.wasPressed()) {
    deactivate_left();
    deactivate_right();
    mode = (mode + 1) % 3;
    for (int i = 0; i < 3; i++) {
      if (mode == 0) {
        M5.Lcd.fillScreen(BLUE);
      } else if (mode == 1) {
        M5.Lcd.fillScreen(GREEN);
      } else if (mode == 2) {
        M5.Lcd.fillScreen(RED);
      }
      delay(100);
      M5.Lcd.fillScreen(BLACK);
      delay(100);
    }
  }

  if (M5.BtnB.wasPressed()) {
    limited_speed += 20;
    if (limited_speed > 255)
      limited_speed = 255;
  }

  if (M5.BtnC.wasPressed()) {
    limited_speed -= 20;
    if (limited_speed < 0)
      limited_speed = 0;
  }
  M5.update();
}
