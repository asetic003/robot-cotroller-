#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Create Access Point (NodeMCU becomes WiFi hotspot)
const char* ap_ssid = "Robot-WiFi";
const char* ap_password = "12345678";

// Create web server
ESP8266WebServer server(80);

// Motor pins
#define LEFT_MOTOR_PIN1 D1
#define LEFT_MOTOR_PIN2 D2
#define LEFT_MOTOR_EN   D3
#define RIGHT_MOTOR_PIN1 D5
#define RIGHT_MOTOR_PIN2 D6
#define RIGHT_MOTOR_EN   D7
#define CUTTER_PIN D8

int leftSpeed = 0;
int rightSpeed = 0;
int powerLimit = 50;
bool cutterState = false;

void setup() {
  Serial.begin(115200);
  
  // Setup pins
  pinMode(LEFT_MOTOR_PIN1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN2, OUTPUT);
  pinMode(LEFT_MOTOR_EN, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN2, OUTPUT);
  pinMode(RIGHT_MOTOR_EN, OUTPUT);
  pinMode(CUTTER_PIN, OUTPUT);
  
  stopMotors();
  digitalWrite(CUTTER_PIN, LOW);
  
  // Create WiFi Access Point
  Serial.println("\nCreating WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point created!");
  Serial.print("Connect to WiFi: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  
  // Setup routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/control", HTTP_OPTIONS, handleOptions);
  server.enableCORS(true);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("========================");
  Serial.println("Enter this IP in web app: " + IP.toString());
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String html = "<html><body><h1>Robot Controller</h1>";
  html += "<p>IP Address: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>Status: Ready</p></body></html>";
  server.send(200, "text/html", html);
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void handleControl() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  leftSpeed = doc["leftMotor"] | 0;
  rightSpeed = doc["rightMotor"] | 0;
  powerLimit = doc["power"] | 50;
  cutterState = doc["cutter"] | 0;
  
  Serial.print("L:");
  Serial.print(leftSpeed);
  Serial.print(" R:");
  Serial.print(rightSpeed);
  Serial.print(" P:");
  Serial.print(powerLimit);
  Serial.print(" C:");
  Serial.println(cutterState);
  
  setLeftMotor(leftSpeed);
  setRightMotor(rightSpeed);
  digitalWrite(CUTTER_PIN, cutterState ? HIGH : LOW);
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void stopMotors() {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  digitalWrite(LEFT_MOTOR_PIN2, LOW);
  digitalWrite(RIGHT_MOTOR_PIN1, LOW);
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
  analogWrite(LEFT_MOTOR_EN, 0);
  analogWrite(RIGHT_MOTOR_EN, 0);
}

void setLeftMotor(int speed) {
  speed = constrain(speed, -100, 100);
  int pwmValue = map(abs(speed), 0, 100, 0, 1023);
  
  if (speed > 0) {
    digitalWrite(LEFT_MOTOR_PIN1, HIGH);
    digitalWrite(LEFT_MOTOR_PIN2, LOW);
  } else if (speed < 0) {
    digitalWrite(LEFT_MOTOR_PIN1, LOW);
    digitalWrite(LEFT_MOTOR_PIN2, HIGH);
  } else {
    digitalWrite(LEFT_MOTOR_PIN1, LOW);
    digitalWrite(LEFT_MOTOR_PIN2, LOW);
  }
  analogWrite(LEFT_MOTOR_EN, pwmValue);
}

void setRightMotor(int speed) {
  speed = constrain(speed, -100, 100);
  int pwmValue = map(abs(speed), 0, 100, 0, 1023);
  
  if (speed > 0) {
    digitalWrite(RIGHT_MOTOR_PIN1, HIGH);
    digitalWrite(RIGHT_MOTOR_PIN2, LOW);
  } else if (speed < 0) {
    digitalWrite(RIGHT_MOTOR_PIN1, LOW);
    digitalWrite(RIGHT_MOTOR_PIN2, HIGH);
  } else {
    digitalWrite(RIGHT_MOTOR_PIN1, LOW);
    digitalWrite(RIGHT_MOTOR_PIN2, LOW);
  }
  analogWrite(RIGHT_MOTOR_EN, pwmValue);
}