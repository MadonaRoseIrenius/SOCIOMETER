#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// LCD Pin mapping - RS, E, D4, D5, D6, D7
LiquidCrystal lcd(15, 4, 14, 27, 26, 25);

// OLED Display settings (128x64, I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 21  // Available pin for SDA
#define OLED_SCL 22  // Available pin for SCL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Access Point credentials
const char* ssid = "ESP32-LCD-Controller";
const char* password = "12345678";

// Create WebServer object on port 80
WebServer server(80);

// Variables to store LCD text and mood
String line1 = "";
String line2 = "";
String currentMood = "neutral";

// Function declarations
void handleRoot();
void handleUpdate();
void handleMood();
void setMoodDisplay(String mood);
String getMoodText(String mood, int line);
void displayEmoji(String mood);

// Mood configurations
struct MoodConfig {
  String name;
  String lcdLine1;
  String lcdLine2;
  String emoji;
};

MoodConfig moods[5] = {
  {"happy", "Feeling Great!", "Have a nice day", "😊"},
  {"sad", "Feeling Down", "Hope gets better", "😢"}, 
  {"angry", "So Annoyed!", "Need some space", "😠"},
  {"excited", "Super Pumped!", "Let's do this!", "🤩"},
  {"neutral", "All Good", "Just chilling", "😐"}
};

// HTML page stored in program memory
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 LCD + OLED Controller</title>
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
            display: flex;
            gap: 20px;
            margin-bottom: 30px;
            flex-wrap: wrap;
        }
        .lcd-display, .oled-display {
            flex: 1;
            min-width: 250px;
        }
        .lcd-display {
            background-color: #001a00;
            color: #00ff00;
            font-family: monospace;
            font-size: 18px;
            padding: 15px;
            border-radius: 8px;
            border: 3px solid #333;
            min-height: 60px;
        }
        .oled-display {
            background-color: #000;
            color: #fff;
            font-family: Arial, sans-serif;
            padding: 20px;
            border-radius: 8px;
            border: 2px solid #444;
            text-align: center;
            min-height: 60px;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .lcd-line {
            display: block;
            height: 25px;
            overflow: hidden;
        }
        .emoji-large {
            font-size: 48px;
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
        <h1>🎮 ESP32 LCD + OLED Controller</h1>
        
        <div class="display-section">
            <div class="lcd-display">
                <span class="lcd-line" id="preview-line1">Ready for input!</span>
                <span class="lcd-line" id="preview-line2"></span>
            </div>
            <div class="oled-display">
                <div class="emoji-large" id="oled-emoji">😐</div>
            </div>
        </div>
        
        <div class="mood-section">
            <h3>🎭 Select Your Mood:</h3>
            <div class="mood-buttons">
                <button class="mood-btn" onclick="selectMood('happy')">😊</button>
                <button class="mood-btn" onclick="selectMood('sad')">😢</button>
                <button class="mood-btn" onclick="selectMood('angry')">😠</button>
                <button class="mood-btn" onclick="selectMood('excited')">🤩</button>
                <button class="mood-btn active" onclick="selectMood('neutral')">😐</button>
            </div>
        </div>
        
        <div class="custom-section">
            <h3>✏️ Custom Text (Optional):</h3>
            <form onsubmit="updateDisplay(); return false;">
                <label for="line1">Line 1 (16 chars max):</label>
                <input type="text" id="line1" maxlength="16" oninput="updatePreview()" placeholder="Leave empty for mood text...">
                <div class="char-count" id="count1">0/16 characters</div>
                
                <label for="line2">Line 2 (16 chars max):</label>
                <input type="text" id="line2" maxlength="16" oninput="updatePreview()" placeholder="Leave empty for mood text...">
                <div class="char-count" id="count2">0/16 characters</div>
                
                <button type="submit">Update Display</button>
                <button type="button" class="clear-btn" onclick="clearDisplay()">Clear All</button>
            </form>
        </div>
        
        <div id="status" class="status"></div>
    </div>

    <script>
        let currentMood = 'neutral';
        
        const moods = {
            'happy': {line1: 'Feeling Great!', line2: 'Have a nice day', emoji: '😊'},
            'sad': {line1: 'Feeling Down', line2: 'Hope gets better', emoji: '😢'},
            'angry': {line1: 'So Annoyed!', line2: 'Need some space', emoji: '😠'},
            'excited': {line1: 'Super Pumped!', line2: "Let's do this!", emoji: '🤩'},
            'neutral': {line1: 'All Good', line2: 'Just chilling', emoji: '😐'}
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
            
            document.getElementById('preview-line1').textContent = line1.padEnd(16, ' ');
            document.getElementById('preview-line2').textContent = line2.padEnd(16, ' ');
            document.getElementById('oled-emoji').textContent = moods[currentMood].emoji;
            
            document.getElementById('count1').textContent = line1Input.length + '/16 characters';
            document.getElementById('count2').textContent = line2Input.length + '/16 characters';
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
            selectMood('neutral');
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
    // Continue anyway
  } else {
    Serial.println("OLED initialized successfully");
  }
  
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Starting WiFi...");
  
  // Initialize OLED with startup message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 Starting...");
  display.display();
  
  // Set up Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Update displays with IP
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: Ready");
  lcd.setCursor(0, 1);
  lcd.print(IP);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Ready!");
  display.println();
  display.print("IP: ");
  display.println(IP);
  display.display();
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/mood", HTTP_POST, handleMood);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  delay(2000);
  
  // Set default mood
  setMoodDisplay("neutral");
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
    
    // Ensure strings don't exceed 16 characters
    if (displayLine1.length() > 16) displayLine1 = displayLine1.substring(0, 16);
    if (displayLine2.length() > 16) displayLine2 = displayLine2.substring(0, 16);
    
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(displayLine1);
    lcd.setCursor(0, 1);
    lcd.print(displayLine2);
    
    // Update OLED with emoji
    displayEmoji(mood);
    
    Serial.println("Displays Updated:");
    Serial.println("LCD Line 1: " + displayLine1);
    Serial.println("LCD Line 2: " + displayLine2);
    Serial.println("OLED Mood: " + mood);
    
    server.send(200, "text/plain", "Displays updated successfully!");
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
  // Update LCD with mood text
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(getMoodText(mood, 1));
  lcd.setCursor(0, 1);
  lcd.print(getMoodText(mood, 2));
  
  // Update OLED with emoji
  displayEmoji(mood);
}

String getMoodText(String mood, int line) {
  for (int i = 0; i < 5; i++) {
    if (moods[i].name == mood) {
      return (line == 1) ? moods[i].lcdLine1 : moods[i].lcdLine2;
    }
  }
  return (line == 1) ? "All Good" : "Just chilling";
}

void displayEmoji(String mood) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  // Center the emoji on the display
  display.setCursor(45, 20);
  
  if (mood == "happy") {
    display.println("^_^");
    display.setTextSize(1);
    display.setCursor(35, 45);
    display.println("HAPPY!");
  } else if (mood == "sad") {
    display.println("T_T");
    display.setTextSize(1);
    display.setCursor(40, 45);
    display.println("SAD...");
  } else if (mood == "angry") {
    display.println(">:(");
    display.setTextSize(1);
    display.setCursor(35, 45);
    display.println("ANGRY!");
  } else if (mood == "excited") {
    display.println("*o*");
    display.setTextSize(1);
    display.setCursor(30, 45);
    display.println("EXCITED!");
  } else { // neutral
    display.println(":|");
    display.setTextSize(1);
    display.setCursor(30, 45);
    display.println("NEUTRAL");
  }
  
  display.display();
}