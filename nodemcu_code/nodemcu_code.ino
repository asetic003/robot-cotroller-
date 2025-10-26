#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

// Create Access Point (NodeMCU becomes WiFi hotspot)
const char* ap_ssid = "Robot-WiFi";
const char* ap_password = "12345678";
DNSServer dnsServer;          // answers * -> 192.168.4.1
const byte DNS_PORT = 53;
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

void startCaptivePortal() {
// 1. DNS hijack: every name → 192.168.4.1
dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
// 2. HTTP hijack: every unknown URI → /welcome
server.onNotFound( {
server.sendHeader("Location", "http://192.168.4.1/", true);
server.send(302, "text/plain", "");
});
// 3. landing page
server.on("/welcome", HTTP_GET, handleRoot);   // reuse your existing page
}

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
  startCaptivePortal();  
  Serial.println("========================");
  Serial.println("Enter this IP in web app: " + IP.toString());
}

void loop() {
  dnsServer.processNextRequest(); 
  server.handleClient();
}

// --- NEW --- Store the web app in program memory
const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <link rel="icon" type="image/png" sizes="192x192" href="icon-192.png">
    <link rel="apple-touch-icon" sizes="192x192" href="icon-192.png">
    <link rel="icon" type="image/png" sizes="512x512" href="icon-512.png">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="theme-color" content="#FFFFFF">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="Robot">
    <link rel="manifest" href="manifest.json">
    <title>Robot Controller</title>
    <style>
        /* --- NEW THEME --- */
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            -webkit-tap-highlight-color: transparent;
        }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #FFFFFF; /* Plain white */
            min-height: 100vh; /* Allow content to expand beyond viewport height */
            display: flex;
            flex-direction: column;
            color: #000000; /* Plain black text */
            touch-action: none;
            /* Removed overflow: hidden; to allow scrolling */
        }

        .setup-screen {
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            height: 100vh;
            padding: 20px;
        }

        .setup-screen.hidden {
            display: none;
        }

        .setup-content {
            /* Removed background and blur */
            padding: 30px;
            border-radius: 20px;
            max-width: 400px;
            width: 100%;
            text-align: center;
            border: 3px solid #000; /* Added border */
        }

        .setup-content h1 {
            font-size: 32px;
            margin-bottom: 20px;
        }

        .setup-content p {
            margin-bottom: 20px;
            opacity: 0.9;
        }

        .ip-input {
            width: 100%;
            padding: 15px;
            border: 2px solid #000; /* Simple border */
            border-radius: 10px;
            font-size: 18px;
            margin-bottom: 15px;
            background: #fff;
            color: #000;
        }

        .connect-btn {
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 18px;
            font-weight: bold;
            background: #000; /* Inverted for primary action */
            color: #fff;
            cursor: pointer;
        }

        .connect-btn:active {
            transform: scale(0.98);
        }

        .connect-btn:disabled {
            background: #888;
            cursor: not-allowed;
        }

        .control-screen {
            display: flex; /* Changed to flex */
            flex-direction: column; /* Arrange children vertically */
            min-height: 100vh; /* Ensure it takes at least full height */
            position: relative;
        }

        .control-screen.active {
            display: block;
        }

        .top-bar {
            position: sticky; /* Changed to sticky */
            top: 0; /* Stick to the top */
            left: 0;
            right: 0;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 5vh 2vw;
            background: #FFFFFF; /* Added background for sticky element */
            border-bottom: 2px solid #000; /* Added border */
            z-index: 100;
            height: 8vh;
        }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 0.5vw;
            /* Removed background */
            padding: 3vh 6vw;
            border-radius: 20px;
            font-size: 3vh;
        }

        .status-dot {
            width: 3vh;
            height: 3vh;
            border-radius: 50%;
            background: #40a351f1; /* Changed to black */
            animation: pulse 2s infinite;
        }

        /* --- NEW --- Added disconnected state */
        .status-dot.disconnected {
            background: #e03c3c; /* Red */
            animation: none;
        }

        .disconnect-btn {
            padding: 0.8vh 2vw;
            border: 2px solid #000; /* Simple border */
            border-radius: 20px;
            background: #fff;
            color: #000;
            font-weight: bold;
            cursor: pointer;
            font-size: 1.8vh;
        }

        .center-info {
            /* Removed absolute positioning */
            text-align: center;
            z-index: 1;
            pointer-events: none;
            margin: auto; /* Center horizontally and vertically within flex container */
            padding: 20px; /* Add some padding */
        }

        .speed-display {
            font-size: 6vh;
            font-weight: bold;
            margin-bottom: 1vh;
            /* Removed text shadow */
        }

        .motor-status {
            display: flex;
            gap: 2vw;
            justify-content: center;
            margin-top: 1vh;
        }

        .motor-info {
            /* Removed background and blur */
            padding: 1vh 2vw;
            border-radius: 10px;
            border: 2px solid #000; /* Added border */
        }

        .motor-label {
            font-size: 1.5vh;
            opacity: 1;
            color: #333; /* Dark gray */
        }

        .motor-value {
            font-size: 3vh;
            font-weight: bold;
        }

        .control-container {
            /* Removed fixed height */
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 20px 30px; /* Adjusted padding */
            flex-grow: 1; /* Allow it to take available space */
        }

        .button-group {
            display: flex;
            flex-direction: column;
            gap: 2vh;
            z-index: 10;
        }

        .button-group.horizontal {
            flex-direction: row;
        }

        .control-btn {
            width: 25vh;
            height: 25vh;
            border: 5% solid #000; /* Strong border */
            border-radius: 100%;
            background: #fff; /* White fill */
            /* Removed backdrop-filter */
            color: #000; /* Black icon/text */
            font-size: 10vh;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.1s;
            /* Removed box-shadow */
            display: flex;
            align-items: center;
            justify-content: center;
            user-select: none;
        }

        /* Invert colors when pressed */
        .control-btn:active,
        .control-btn.pressed {
            transform: scale(0.95);
            background: #000;
            color: #fff;
        }

        .bottom-controls {
            position: sticky; /* Changed to sticky */
            bottom: 0; /* Stick to the bottom */
            left: 0;
            right: 0;
            display: flex;
            justify-content: space-around;
            align-items: center;
            padding: 5vh;
            background: #FFFFFF; /* Added background for sticky element */
            border-top: 2px solid #000000; /* Added border */
            margin-top: auto; /* Push to bottom if content is short */
        }

        .power-label {
            font-size: 13px;
            font-weight: 500;
            color: #86868b;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            white-space: nowrap;
        }
        
        .power-slider {
            flex: 1;
            height: 6px;
            -webkit-appearance: none;
            appearance: none;
            background: #e5e5e7;
            border-radius: 3px;
            border: none;
            outline: none;
            transition: background 0.2s ease;
        }
        
        .power-slider:hover {
            background: #d2d2d7;
        }
        
        .power-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            background: #000;
            border-radius: 50%;
            cursor: pointer;
            transition: transform 0.15s ease, box-shadow 0.15s ease;
        }
        
        .power-slider::-webkit-slider-thumb:hover {
            transform: scale(1.1);
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
        }
        
        .power-slider::-webkit-slider-thumb:active {
            transform: scale(1.05);
            box-shadow: 0 1px 4px rgba(0, 0, 0, 0.3);
        }
        
        .power-slider::-moz-range-thumb {
            width: 24px;
            height: 24px;
            background: #000;
            border-radius: 50%;
            cursor: pointer;
            border: none;
            transition: transform 0.15s ease, box-shadow 0.15s ease;
        }
        
        .power-slider::-moz-range-thumb:hover {
            transform: scale(1.1);
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
        }
        
        .power-slider::-moz-range-thumb:active {
            transform: scale(1.05);
            box-shadow: 0 1px 4px rgba(0, 0, 0, 0.3);
        }
        
        .power-value {
            font-size: 18px;
            font-weight: 600;
            color: #1d1d1f;
            min-width: 50px;
            text-align: right;
            font-variant-numeric: tabular-nums;
        }
        
        .power-unit {
            font-size: 14px;
            font-weight: 400;
            color: #86868b;
        }
        .cutter-btn {
            /* Converted px values to vh for scaling */
            padding: 2.5vh 5vh;   /* Was 15px 30px */
            border: 0.3vh solid #000; /* Was 2px */
            border-radius: 1.5vh;   /* Was 15px */
            font-size: 2.5vh;     /* Was 16px */
            
            /* Unchanged properties */
            font-weight: bold;
            cursor: pointer;
            background: #fff; /* White background */
            color: #000; /* Black text */
            transition: all 0.3s;
        }
        
        /* Invert colors when active */
        .cutter-btn.active {
            background: #000;
            color: #fff;
            /* Removed box-shadow */
        }

        .cutter-btn:active {
            transform: scale(0.95);
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        /* Portrait mode warning */
        @media (orientation: portrait) {
            .portrait-warning {
                display: flex;
                position: fixed;
                top: 0;
                left: 0;
                right: 0;
                bottom: 0;
                background: #fff; /* White background */
                color: #000; /* Black text */
                justify-content: center;
                align-items: center;
                z-index: 1000;
                flex-direction: column;
                gap: 20px;
            }

            .portrait-warning .icon {
                font-size: 80px;
            }

            .portrait-warning h2 {
                font-size: 24px;
            }
        }

        @media (orientation: landscape) {
            .portrait-warning {
                display: none !important;
            }
        }
    </style>
</head>
<body>
    
    <div class="portrait-warning">
        <div class="icon">📱</div>
        <h2>Please rotate your device</h2>
        <p>This app works best in landscape mode</p>
    </div>

    <div class="setup-screen" id="setupScreen">
        <div class="setup-content">
            <h1>🤖 Robot Controller</h1>
            <p>Click the button to start controlling your robot.</p>
            <button class="connect-btn" id="connectBtn" onclick="connect()">Start</button>
        </div>
    </div>

    <div class="control-screen" id="controlScreen">
        <div class="top-bar">
            <div class="status-badge">
                <div class="status-dot"></div>
                <span id="ipDisplay">Connected</span>
            </div>
            <button class="disconnect-btn" onclick="disconnect()">Disconnect</button>
        </div>

        <div class="center-info">
            <div class="speed-display" id="speedDisplay">0%</div>
            <div class="motor-status">
                <div class="motor-info">
                    <div class="motor-label">LEFT</div>
                    <div class="motor-value" id="leftMotor">0%</div>
                </div>
                <div class="motor-info">
                    <div class="motor-label">RIGHT</div>
                    <div class="motor-value" id="rightMotor">0%</div>
                </div>
            </div>
        </div>

        <div class="control-container">
            <div class="button-group horizontal">
                <button class="control-btn" id="leftBtn">
                    ←
                </button>
                <button class="control-btn" id="rightBtn">
                    →
                </button>
            </div>

            <div class="button-group">
                <button class="control-btn" id="forwardBtn">
                    ↑
                </button>
                <button class="control-btn" id="backwardBtn">
                    ↓
                </button>
            </div>
        </div>

        <div class="bottom-controls">
            <div class="power-control">
                <span class="power-label">Power:</span>
                <input type="range" class="power-slider" id="powerSlider" min="0" max="100" value="50">
                <span class="power-value"><span id="powerValue">50</span><span class="power-unit">%</span></span>
            </div>
            <button class="cutter-btn" id="cutterBtn" onclick="toggleCutter()">
                ⚠️ CUTTER OFF
            </button>
        </div>
    </div>

    <script>
      let connected = false;
      let cutterOn = false;
      let nodeIP = ''; // This will be empty as we are on the device
      let currentPower = 50;
      let turningFactor = 0.6;
      let leftMotorSpeed = 0;
      let rightMotorSpeed = 0;
          
      let forwardPressed = false;
      let backwardPressed = false;
      let leftPressed = false;
      let rightPressed = false;
          
      // Acceleration variables
      let currentForward = 0; // Current acceleration level (-100 to 100)
      let currentTurn = 0; // Current turn level (-100 to 100)
      let accelerationRate = 3; // How fast to accelerate (units per frame)
      let decelerationRate = 5; // How fast to decelerate (units per frame)
      let accelerationInterval = null;

      // --- NEW --- Heartbeat variables
      let heartbeatInterval = null;
      const statusDot = document.querySelector('.status-dot');
      const ipDisplay = document.getElementById('ipDisplay');
      // --- END NEW ---
          
      const buttons = {
          forward: document.getElementById('forwardBtn'),
          backward: document.getElementById('backwardBtn'),
          left: document.getElementById('leftBtn'),
          right: document.getElementById('rightBtn')
      };
      
      function setupButton(button, onPress, onRelease) {
          // Touch events
          button.addEventListener('touchstart', (e) => {
              e.preventDefault();
              button.classList.add('pressed');
              onPress();
          });
      
          button.addEventListener('touchend', (e) => {
              e.preventDefault();
              button.classList.remove('pressed');
              onRelease();
          });
      
          // Mouse events for testing on desktop
          button.addEventListener('mousedown', (e) => {
              e.preventDefault();
              button.classList.add('pressed');
              onPress();
          });
      
          button.addEventListener('mouseup', (e) => {
              e.preventDefault();
              button.classList.remove('pressed');
              onRelease();
          });
      
          button.addEventListener('mouseleave', (e) => {
              if (button.classList.contains('pressed')) {
                button.classList.remove('pressed');
                onRelease();
              }
          });
      }
      
      setupButton(buttons.forward, 
          () => { forwardPressed = true; startAcceleration(); },
          () => { forwardPressed = false; startAcceleration(); }
      );
      
      setupButton(buttons.backward,
          () => { backwardPressed = true; startAcceleration(); },
          () => { backwardPressed = false; startAcceleration(); }
      );
      
      setupButton(buttons.left,
          () => { leftPressed = true; startAcceleration(); },
          () => { leftPressed = false; startAcceleration(); }
      );
      
      setupButton(buttons.right,
          () => { rightPressed = true; startAcceleration(); },
          () => { rightPressed = false; startAcceleration(); }
      );
      
      function startAcceleration() {
          // Clear existing interval if any
          if (accelerationInterval) {
              clearInterval(accelerationInterval);
          }
          
          // Start acceleration loop
          accelerationInterval = setInterval(() => {
              updateAcceleration();
          }, 50); // Update every 50ms (20 times per second)
      }
      
      function updateAcceleration() {
          // Determine target forward speed
          let targetForward = 0;
          if (forwardPressed) targetForward = 100;
          if (backwardPressed) targetForward = -100;
          
          // Determine target turn speed
          let targetTurn = 0;
          if (leftPressed) targetTurn = -100;
          if (rightPressed) targetTurn = 100;
          
          // Gradually move current values toward targets
          if (currentForward < targetForward) {
              currentForward = Math.min(currentForward + accelerationRate, targetForward);
          } else if (currentForward > targetForward) {
              currentForward = Math.max(currentForward - decelerationRate, targetForward);
          }
          
          if (currentTurn < targetTurn) {
              currentTurn = Math.min(currentTurn + accelerationRate, targetTurn);
          } else if (currentTurn > targetTurn) {
              currentTurn = Math.max(currentTurn - decelerationRate, targetTurn);
          }
          
          // Update motors with current acceleration values
          updateMotors();
          
          // Stop interval if we've reached target and no buttons pressed
          if (currentForward === targetForward && currentTurn === targetTurn && 
              !forwardPressed && !backwardPressed && !leftPressed && !rightPressed) {
              clearInterval(accelerationInterval);
              accelerationInterval = null;
          }
      }
      
      function updateMotors() {
          const powerMultiplier = currentPower / 100;
          let left = 0;
          let right = 0;
      
          // --- Smooth differential steering with gradual acceleration ---
          if (currentForward !== 0) {
              // Moving forward or backward with optional turn
              if (currentTurn === 0) {
                  left = right = currentForward; // straight
              } else if (currentTurn > 0) {
                  // turning right
                  left = currentForward;
                  right = currentForward * (1 - turningFactor * (Math.abs(currentTurn) / 100));
              } else if (currentTurn < 0) {
                  // turning left
                  left = currentForward * (1 - turningFactor * (Math.abs(currentTurn) / 100));
                  right = currentForward;
              }
          } else if (currentTurn !== 0) {
              // In-place turning (no forward motion)
              left = currentTurn;
              right = -currentTurn;
          } else {
              // stop
              left = right = 0;
          }
      
          // Apply power scaling
          leftMotorSpeed = Math.round(left * powerMultiplier);
          rightMotorSpeed = Math.round(right * powerMultiplier);
      
          // Clamp values
          leftMotorSpeed = Math.max(-100, Math.min(100, leftMotorSpeed));
          rightMotorSpeed = Math.max(-100, Math.min(100, rightMotorSpeed));
      
          // Update display
          document.getElementById('leftMotor').textContent = leftMotorSpeed + '%';
          document.getElementById('rightMotor').textContent = rightMotorSpeed + '%';
      
          const avgSpeed = Math.abs(Math.round((leftMotorSpeed + rightMotorSpeed) / 2));
          document.getElementById('speedDisplay').textContent = avgSpeed + '%';
      
          sendCommand();
      }
      
      const powerSlider = document.getElementById('powerSlider');
      const powerValue = document.getElementById('powerValue');
      
      powerSlider.addEventListener('input', (e) => {
          currentPower = parseInt(e.target.value);
          powerValue.textContent = currentPower;
          updateMotors();
      });
      
      // --- MODIFIED --- Connect function is now much simpler
      function connect() {
          connected = true;
          ipDisplay.textContent = 'Connected';
          statusDot.classList.remove('disconnected');
          document.getElementById('setupScreen').classList.add('hidden');
          document.getElementById('controlScreen').classList.add('active');
          startHeartbeat(); // Start connection polling
      }
      
      function disconnect() {
          connected = false;
          nodeIP = '';
          clearInterval(heartbeatInterval); // --- NEW --- Stop heartbeat
          
          // Clear acceleration
          if (accelerationInterval) {
              clearInterval(accelerationInterval);
              accelerationInterval = null;
          }
          currentForward = 0;
          currentTurn = 0;
          leftMotorSpeed = 0;
          rightMotorSpeed = 0;
          cutterOn = false;
          
          // Don't send command, just reset UI
          document.getElementById('setupScreen').classList.remove('hidden');
          document.getElementById('controlScreen').classList.remove('active');
          document.getElementById('cutterBtn').classList.remove('active');
          document.getElementById('cutterBtn').textContent = '⚠️ CUTTER OFF';

          // --- NEW --- Reset status dot
          ipDisplay.textContent = 'Connected'; 
          statusDot.classList.remove('disconnected');
      }
      
      function toggleCutter() {
          if (!connected) return;
          cutterOn = !cutterOn;
          const btn = document.getElementById('cutterBtn');
          if (cutterOn) {
              btn.classList.add('active');
              btn.textContent = '⚠️ CUTTER ON';
          } else {
              btn.classList.remove('active');
              btn.textContent = '⚠️ CUTTER OFF';
          }
          sendCommand();
      }
      
      // --- MODIFIED --- sendCommand now sends to a relative URL
      function sendCommand() {
        if (!connected) return;

        const data = { 
            leftMotor: leftMotorSpeed, 
            rightMotor: rightMotorSpeed, 
            power: currentPower, 
            cutter: cutterOn ? 1 : 0 
        };
        
        fetch(`/control`, { // Use relative URL
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify(data)
        }).catch(err => {
            console.error('Send command failed:', err);
            handleConnectionLoss();
        });
      }       

      // --- NEW --- Heartbeat and connection loss functions
      function startHeartbeat() {
          clearInterval(heartbeatInterval); // Clear any old one
          heartbeatInterval = setInterval(checkConnection, 3000); // Check every 3 seconds
      }

      async function checkConnection() {
          if (!connected) {
              clearInterval(heartbeatInterval);
              return;
          }
          
          try {
              // Ping the server. If it fails, it will throw an error.
              await fetch(`/`, { // Use relative URL
                  method: 'GET',
                  signal: AbortSignal.timeout(2000) // 2-second timeout for pings
              });
              
              // If we were disconnected and it succeeds, mark as reconnected
              if (statusDot.classList.contains('disconnected')) {
                  console.log('Reconnected to robot!');
                  connected = true; // Set back to true
                  ipDisplay.textContent = 'Connected';
                  statusDot.classList.remove('disconnected');
              }

          } catch (err) {
              // If ping fails, handle connection loss
              console.log('Heartbeat failed, connection lost.');
              handleConnectionLoss();
          }
      }

      function handleConnectionLoss() {
          if (!connected) return; // Only trigger once

          console.log('Handling connection loss...');
          connected = false; // Stop sendCommand from trying
          clearInterval(heartbeatInterval); // Stop trying to ping
          
          // Update UI to show disconnected state
          ipDisplay.textContent = 'Disconnected';
          statusDot.classList.add('disconnected');
          
          // Stop robot by clearing acceleration interval
          if (accelerationInterval) {
              clearInterval(accelerationInterval);
              accelerationInterval = null;
          }
          currentForward = 0;
          currentTurn = 0;
          // We can't send a "stop" command as the connection is lost
          // But we can update the UI to show 0%
          document.getElementById('leftMotor').textContent = '0%';
          document.getElementById('rightMotor').textContent = '0%';
          document.getElementById('speedDisplay').textContent = '0%';
      }
      // --- END NEW ---
      
      // Prevent accidental page navigation
      document.addEventListener('touchmove', (e) => {
          if (connected) e.preventDefault();
      }, { passive: false });
      
      // Register service worker for PWA
      if ('serviceWorker' in navigator) {
          navigator.serviceWorker.register('service-worker.js')
              .then(reg => console.log('Service Worker registered'))
              .catch(err => console.log('Service Worker registration failed'));
      }
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  // Serve the web app from PROGMEM
  server.send_P(200, "text/html", INDEX_HTML);
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