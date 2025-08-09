#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display settings (128x64, I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 21  // Available pin for SDA
#define OLED_SCL 22  // Available pin for SCL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Access Point credentials
const char* ssid = "SOCIOMETER";
const char* password = "12345678";

// Create WebServer object on port 80
WebServer server(80);

// Variables to store display text and mood
String line1 = "";
String line2 = "";
String currentMood = "none";

// Function declarations
void handleRoot();
void handleUpdate();
void handleMood();
void setMoodDisplay(String mood);
String getMoodText(String mood, int line);
void displayContent(String mood, String text1, String text2);
void animateHappy();
void animateSad();
void animateAngry();
void animateExcited();
void animateNeutral();
void animateNone();

// Mood configurations
struct MoodConfig {
  String name;
  String lcdLine1;
  String lcdLine2;
  String emoji;
};

MoodConfig moods[6] = {
  {"happy", "Feeling Great!", "Have a nice day", "^_^"},
  {"sad", "Feeling Down", "Hope gets better", "T_T"}, 
  {"angry", "So Annoyed!", "Need some space", ">:("},
  {"excited", "Super Pumped!", "Let's do this!", "*o*"},
  {"neutral", "All Good", "Just chilling", ":|"},
  {"none", "Not set", "", "?"}
};

// HTML page stored in program memory
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OLED Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 20px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
        }
        .container {
            background-color: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
            font-size: 28px;
        }
        .display-section {
            margin-bottom: 30px;
            display: flex;
            justify-content: center;
        }
        .oled-display {
            background-color: #000;
            color: #fff;
            font-family: monospace;
            padding: 20px;
            border-radius: 8px;
            border: 2px solid #444;
            width: 300px;
            height: 150px;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
        }
        .emoji-display {
            font-size: 32px;
            margin-bottom: 10px;
        }
        .text-line {
            font-size: 14px;
            margin: 2px 0;
            text-align: center;
        }
        .mood-section {
            margin-bottom: 30px;
        }
        .mood-buttons {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
            justify-content: center;
            margin-bottom: 20px;
        }
        .mood-btn {
            background: linear-gradient(45deg, #ff6b6b, #ee5a24);
            color: white;
            border: none;
            padding: 15px 20px;
            border-radius: 50px;
            cursor: pointer;
            font-size: 24px;
            transition: all 0.3s ease;
            min-width: 80px;
        }
        .mood-btn:hover {
            transform: scale(1.1);
            box-shadow: 0 5px 15px rgba(0,0,0,0.3);
        }
        .mood-btn.active {
            background: linear-gradient(45deg, #2ed573, #1e90ff);
            transform: scale(1.05);
        }
        .custom-section {
            border-top: 2px solid #eee;
            padding-top: 20px;
        }
        input[type="text"] {
            width: 100%;
            padding: 12px;
            font-size: 16px;
            border: 2px solid #ddd;
            border-radius: 8px;
            margin: 10px 0;
            box-sizing: border-box;
            transition: border-color 0.3s ease;
        }
        input[type="text"]:focus {
            border-color: #667eea;
            outline: none;
        }
        button {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            padding: 12px 24px;
            font-size: 16px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            width: 100%;
            margin: 5px 0;
            transition: all 0.3s ease;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .clear-btn {
            background: linear-gradient(45deg, #ff6b6b, #ee5a24);
        }
        .char-count {
            font-size: 12px;
            color: #666;
            margin-top: 5px;
        }
        .status {
            margin-top: 20px;
            padding: 10px;
            border-radius: 8px;
            text-align: center;
            display: none;
        }
        .success {
            background-color: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .error {
            background-color: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        h3 {
            color: #333;
            margin-bottom: 15px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>SOCIOMETER</h1>
        
        <div class="display-section">
            <div class="oled-display">
                <div class="emoji-display" id="oled-emoji">❓</div>
                <div class="text-line" id="preview-line1">Not set</div>
                <div class="text-line" id="preview-line2"></div>
            </div>
        </div>
        
        <div class="mood-section">
            <h3>🎭 Select Your Mood:</h3>
            <div class="mood-buttons">
                <button class="mood-btn" onclick="selectMood('happy')">Happy ^-^</button>
                <button class="mood-btn" onclick="selectMood('sad')">Sad T-T</button>
                <button class="mood-btn" onclick="selectMood('angry')">Angry ):(</button>
                <button class="mood-btn" onclick="selectMood('excited')">Excited *o*</button>
                <button class="mood-btn" onclick="selectMood('neutral')">Nuetral : |</button>
            </div>
        </div>
        
        <div class="custom-section">
            <h3>Custom Text (Optional):</h3>
            <form onsubmit="updateDisplay(); return false;">
                <label for="line1">Line 1 (21 chars max):</label>
                <input type="text" id="line1" maxlength="21" oninput="updatePreview()" placeholder="Leave empty for mood text...">
                <div class="char-count" id="count1">0/21 characters</div>
                
                <label for="line2">Line 2 (21 chars max):</label>
                <input type="text" id="line2" maxlength="21" oninput="updatePreview()" placeholder="Leave empty for mood text...">
                <div class="char-count" id="count2">0/21 characters</div>
                
                <button type="submit">Update Display</button>
                <button type="button" class="clear-btn" onclick="clearDisplay()">Clear All</button>
            </form>
        </div>
        
        <div id="status" class="status"></div>
    </div>

    <script>
        let currentMood = 'none';
        
        const moods = {
            'happy': {line1: 'Feeling Great!', line2: 'Have a nice day', emoji: '😊'},
            'sad': {line1: 'Feeling Down', line2: 'Hope gets better', emoji: '😢'},
            'angry': {line1: 'So Annoyed!', line2: 'Need some space', emoji: '😠'},
            'excited': {line1: 'Super Pumped!', line2: "Let's do this!", emoji: '🤩'},
            'neutral': {line1: 'All Good', line2: 'Just chilling', emoji: '😐'},
            'none': {line1: 'Not set', line2: '', emoji: '❓'}
        };
        
        function selectMood(mood) {
            currentMood = mood;
            
            // Update button states
            document.querySelectorAll('.mood-btn').forEach(btn => btn.classList.remove('active'));
            event.target.classList.add('active');
            
            // Update displays
            updatePreview();
            
            // Send mood to ESP32
            fetch('/mood', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'mood=' + encodeURIComponent(mood)
            });
        }
        
        function updatePreview() {
            const line1Input = document.getElementById('line1').value;
            const line2Input = document.getElementById('line2').value;
            
            // Use custom text if provided, otherwise use mood text
            const line1 = line1Input || moods[currentMood].line1;
            const line2 = line2Input || moods[currentMood].line2;
            
            document.getElementById('preview-line1').textContent = line1;
            document.getElementById('preview-line2').textContent = line2;
            document.getElementById('oled-emoji').textContent = moods[currentMood].emoji;
            
            document.getElementById('count1').textContent = line1Input.length + '/21 characters';
            document.getElementById('count2').textContent = line2Input.length + '/21 characters';
        }
        
        function updateDisplay() {
            const line1 = document.getElementById('line1').value;
            const line2 = document.getElementById('line2').value;
            
            fetch('/update', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: `line1=${encodeURIComponent(line1)}&line2=${encodeURIComponent(line2)}&mood=${encodeURIComponent(currentMood)}`
            })
            .then(response => response.text())
            .then(data => {
                showStatus('Display updated successfully!', 'success');
            })
            .catch(error => {
                showStatus('Error updating display', 'error');
            });
        }
        
        function clearDisplay() {
            document.getElementById('line1').value = '';
            document.getElementById('line2').value = '';
            selectMood('none');
            updateDisplay();
        }
        
        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.textContent = message;
            status.className = 'status ' + type;
            status.style.display = 'block';
            setTimeout(() => {
                status.style.display = 'none';
            }, 3000);
        }
        
        // Initialize
        updatePreview();
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C for OLED (SDA=21, SCL=22)
  Wire.begin(21, 22);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while(true); // Don't proceed if OLED fails
  } else {
    Serial.println("OLED initialized successfully");
  }
  
  // Initialize OLED with startup message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Starting...");
  display.println();
  display.println("Initializing WiFi...");
  display.display();
  
  // Set up Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Update OLED with connection info
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("WiFi Ready!");
  display.println();
  display.print("SSID: ");
  display.println(ssid);
  display.println();
  display.print("IP: ");
  display.println(IP);
  display.println();
  display.println("Connect and browse to");
  display.println("the IP address above");
  display.display();
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/mood", HTTP_POST, handleMood);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  delay(3000);
  
  // Set default to "not set" state
  setMoodDisplay("none");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleUpdate() {
  if (server.hasArg("mood")) {
    String mood = server.arg("mood");
    String customLine1 = server.arg("line1");
    String customLine2 = server.arg("line2");
    
    // Use custom text if provided, otherwise use mood text
    String displayLine1 = customLine1.length() > 0 ? customLine1 : getMoodText(mood, 1);
    String displayLine2 = customLine2.length() > 0 ? customLine2 : getMoodText(mood, 2);
    
    // Ensure strings don't exceed reasonable length for OLED
    if (displayLine1.length() > 21) displayLine1 = displayLine1.substring(0, 21);
    if (displayLine2.length() > 21) displayLine2 = displayLine2.substring(0, 21);
    
    // Update OLED with mood and text
    displayContent(mood, displayLine1, displayLine2);
    
    Serial.println("OLED Display Updated:");
    Serial.println("Line 1: " + displayLine1);
    Serial.println("Line 2: " + displayLine2);
    Serial.println("Mood: " + mood);
    
    server.send(200, "text/plain", "Display updated successfully!");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleMood() {
  if (server.hasArg("mood")) {
    String mood = server.arg("mood");
    currentMood = mood;
    setMoodDisplay(mood);
    server.send(200, "text/plain", "Mood updated!");
  } else {
    server.send(400, "text/plain", "Missing mood parameter");
  }
}

void setMoodDisplay(String mood) {
  String line1 = getMoodText(mood, 1);
  String line2 = getMoodText(mood, 2);
  displayContent(mood, line1, line2);
}

String getMoodText(String mood, int line) {
  for (int i = 0; i < 6; i++) {
    if (moods[i].name == mood) {
      return (line == 1) ? moods[i].lcdLine1 : moods[i].lcdLine2;
    }
  }
  return (line == 1) ? "Not set" : "";
}

void displayContent(String mood, String text1, String text2) {
  // Play mood animation first
  if (mood == "happy") {
    animateHappy();
  } else if (mood == "sad") {
    animateSad();
  } else if (mood == "angry") {
    animateAngry();
  } else if (mood == "excited") {
    animateExcited();
  } else if (mood == "neutral") {
    animateNeutral();
  } else if (mood == "none") {
    animateNone();
  }
  
  // After animation, show the final static display
  delay(500);
  
  display.clearDisplay();
  
  // Display emoji at the top center
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  
  String emoji = "";
  for (int i = 0; i < 6; i++) {
    if (moods[i].name == mood) {
      emoji = moods[i].emoji;
      break;
    }
  }
  
  // Center the emoji horizontally
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(emoji, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 5);
  display.println(emoji);
  
  // Display text lines
  display.setTextSize(1);
  
  // First line of text
  display.getTextBounds(text1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 35);
  display.println(text1);
  
  // Second line of text
  if (text2.length() > 0) {
    display.getTextBounds(text2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 50);
    display.println(text2);
  }
  
  display.display();
}

void animateHappy() {
  // Bouncing happy face animation
  for (int bounce = 0; bounce < 3; bounce++) {
    for (int y = 0; y <= 10; y += 2) {
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(45, 15 + y);
      display.println("^_^");
      display.display();
      delay(50);
    }
    for (int y = 10; y >= 0; y -= 2) {
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(45, 15 + y);
      display.println("^_^");
      display.display();
      delay(50);
    }
  }
}

void animateSad() {
  // Falling tear animation
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(45, 15);
  display.println("T_T");
  display.display();
  delay(500);
  
  // Animate falling tears
  for (int drop = 0; drop < 3; drop++) {
    for (int y = 35; y <= 55; y += 3) {
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(45, 15);
      display.println("T_T");
      
      // Draw tear drops
      display.setTextSize(1);
      display.setCursor(55, y);
      display.println(".");
      display.setCursor(65, y - 5);
      display.println(".");
      
      display.display();
      delay(100);
    }
    delay(200);
  }
}

void animateAngry() {
  // Shaking angry face
  for (int shake = 0; shake < 8; shake++) {
    display.clearDisplay();
    display.setTextSize(3);
    int xOffset = (shake % 2 == 0) ? 42 : 48;
    display.setCursor(xOffset, 15);
    display.println(">:(");
    
    // Add anger lines
    display.setTextSize(1);
    if (shake % 2 == 0) {
      display.setCursor(30, 10);
      display.println("!");
      display.setCursor(90, 12);
      display.println("!");
    }
    
    display.display();
    delay(150);
  }
}

void animateExcited() {
  // Pulsing excited face with stars
  for (int pulse = 0; pulse < 4; pulse++) {
    // Small size
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(50, 20);
    display.println("*o*");
    display.display();
    delay(200);
    
    // Large size with stars
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(45, 15);
    display.println("*o*");
    
    // Add sparkle stars
    display.setTextSize(1);
    display.setCursor(20, 10);
    display.println("*");
    display.setCursor(100, 15);
    display.println("*");
    display.setCursor(25, 40);
    display.println("*");
    display.setCursor(95, 35);
    display.println("*");
    
    display.display();
    delay(200);
  }
}

void animateNeutral() {
  // Simple fade in animation
  for (int fade = 0; fade < 3; fade++) {
    display.clearDisplay();
    if (fade == 1) {
      display.setTextSize(3);
      display.setCursor(48, 15);
      display.println(":|");
    }
    display.display();
    delay(300);
  }
}

void animateNone() {
  // Question mark with blinking effect
  for (int blink = 0; blink < 4; blink++) {
    display.clearDisplay();
    if (blink % 2 == 0) {
      display.setTextSize(3);
      display.setCursor(55, 15);
      display.println("?");
    }
    display.display();
    delay(400);
  }
}