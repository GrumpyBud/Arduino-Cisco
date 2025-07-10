#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_WIDTH 128
#define TFT_HEIGHT 128

const char* ssid = "hsi";
const char* password = "zAFwr*yxc2s2dgWT";

WebServer server(80);

const int pinButton = 10;
const int pinLed = 22;
const int pinBuzz = 9;
const int pinSelect = 12;
const int pinBackspace = 14;
const int pinEnter = 27;
const int pinShift = 33;
const int pinJoyX = 34;
const int pinJoyY = 35;

const int centerVal = 2048;
const int deadzoneVal = 700;

const char* letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const String morseCodeTable[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."
};

const unsigned long dotThresh = 300;
const unsigned long symbolGapThresh = 600;
const unsigned long letterGapThresh = 1000;
const unsigned long wordGapThresh = 1400;

const char keyboard[3][11] = {
  "QWERTYUIOP",
  "ASDFGHJKL<",
  "ZXCVBNM_#"
};
const int keyRows = 3;
const int keyCols = 11;

const int buzChan = 0;
const int buzFreq = 1500;
const int buzRes = 8;
const int buzVol = 32;

enum Mode { start, input, output };
Mode mode = start;

int menuCursor = 0;
bool lastButtonState = HIGH;
unsigned long lastChangeTime = 0;
String currentSymbol = "";
String decodedMessage = "";

int cursorX = 0, cursorY = 0;
String typed = "";
unsigned long lastTouch = 0;
const unsigned long dbTime = 200;

float speedMultiplier = 1.0;

void showHomepage();
void homepageLoop();
void initMode();
void morseLoop();
void handlePress(unsigned long);
void handleRelease(unsigned long);
void endLetter();
char decodeMorse(const String&);
void drawMorseMessage();
void keyboardLoop();
void selectKey();
void drawKeyboard();
void drawTextArea();
void translateToMorse(const char*);
String getMorseCode(char);
void showTranslationUI(String, String);
void blinkSymbol(char);
void buzzerOn();
void buzzerOff();

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Morse Code Translator</title>
  <style>
    :root {
      --primary-glow: #00ff99;
      --primary-bg: #0d1117;
      --secondary-bg: rgba(22, 27, 34, 0.75);
      --border-color: rgba(255, 255, 255, 0.1);
      --text-color: #e6edf3;
      --muted-text: #7d8590;
      --input-bg: #010409;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji";
      background-color: var(--primary-bg);
      color: var(--text-color);
      display: grid;
      place-items: center;
      min-height: 100vh;
      margin: 0;
      padding: 1rem;
      background-image: radial-gradient(circle at 1px 1px, rgba(255, 255, 255, 0.04) 1px, transparent 0);
      background-size: 20px 20px;
    }
    .container {
      max-width: 450px;
      width: 100%;
      background: var(--secondary-bg);
      backdrop-filter: blur(12px) saturate(180%);
      -webkit-backdrop-filter: blur(12px) saturate(180%);
      border-radius: 16px;
      border: 1px solid var(--border-color);
      padding: 2.5rem;
      box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
    }
    h1 {
      color: var(--primary-glow);
      text-align: center;
      font-size: 1.75rem;
      margin-top: 0;
      margin-bottom: 2rem;
      letter-spacing: 1px;
      text-shadow: 0 0 10px rgba(0, 255, 153, 0.3);
    }
    form {
      display: flex;
      flex-direction: column;
      gap: 1rem;
      width: 100%;
    }
    .formHeader {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 0.5rem;
      color: var(--text-color);
      font-weight: 600;
    }
    .divider {
      height: 1px;
      width: 100%;
      background: var(--border-color);
      margin: 2rem 0;
    }
    input[type="text"] {
      padding: 0.85rem;
      font-size: 1rem;
      border: 1px solid var(--border-color);
      border-radius: 8px;
      background: var(--input-bg);
      color: var(--text-color);
      transition: border-color 0.3s, box-shadow 0.3s;
    }
    input[type="text"]:focus {
      outline: none;
      border-color: var(--primary-glow);
      box-shadow: 0 0 0 3px rgba(0, 255, 153, 0.2);
    }
    .speedControl {
      display: flex;
      align-items: center;
      gap: 1rem;
      background: var(--input-bg);
      padding: 0.75rem 1rem;
      border-radius: 8px;
      border: 1px solid var(--border-color);
    }
    .speedControl input[type="range"] {
      flex-grow: 1;
      -webkit-appearance: none;
      appearance: none;
      background: transparent;
      cursor: pointer;
    }
    .speedControl input[type="range"]::-webkit-slider-runnable-track {
        background: #333;
        height: 4px;
        border-radius: 2px;
    }
    .speedControl input[type="range"]::-moz-range-track {
        background: #333;
        height: 4px;
        border-radius: 2px;
    }
    .speedControl input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        margin-top: -6px;
        background-color: var(--primary-glow);
        height: 16px;
        width: 16px;
        border-radius: 50%;
        box-shadow: 0 0 5px var(--primary-glow);
    }
    .speedControl input[type="range"]::-moz-range-thumb {
        border: none;
        background-color: var(--primary-glow);
        height: 16px;
        width: 16px;
        border-radius: 50%;
        box-shadow: 0 0 5px var(--primary-glow);
    }
    .speedControl input[type="number"] {
      width: 75px;
      text-align: center;
      font-weight: 600;
      background-color: var(--input-bg);
      color: var(--primary-glow);
      border: none;
      border-radius: 6px;
      padding: 0.25rem;
      font-size: 1rem;
      -moz-appearance: textfield;
    }
    .speedControl input[type="number"]::-webkit-outer-spin-button,
    .speedControl input[type="number"]::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
    }
    .speedControl input[type="number"]:focus {
        outline: 1px solid var(--primary-glow);
    }
    button {
      padding: 0.85rem;
      font-size: 1rem;
      font-weight: 600;
      letter-spacing: 0.5px;
      border: none;
      border-radius: 8px;
      background: linear-gradient(45deg, #00ff99, #00c2a3);
      color: #010409;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 15px rgba(0, 255, 153, 0.25);
    }
    button:active {
      transform: translateY(0);
    }
    .helpTooltip {
      position: relative;
    }
    .helpIcon {
      display: grid;
      place-items: center;
      width: 22px;
      height: 22px;
      background: #2a3038;
      color: var(--muted-text);
      border: 1px solid var(--border-color);
      border-radius: 50%;
      cursor: help;
      font-weight: 700;
      font-size: 0.9rem;
      user-select: none;
    }
    .helpIcon:focus {
        outline: none;
        box-shadow: 0 0 0 3px rgba(0, 255, 153, 0.2);
    }
    .tooltipText {
      position: absolute;
      visibility: hidden;
      opacity: 0;
      width: 220px;
      background-color: #161b22;
      color: var(--text-color);
      text-align: center;
      border-radius: 8px;
      padding: 0.75rem;
      z-index: 10;
      bottom: 140%;
      left: 50%;
      transform: translateX(-50%);
      transition: opacity 0.3s ease, visibility 0.3s ease;
      font-size: 0.85rem;
      font-weight: 400;
      border: 1px solid var(--border-color);
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
    }
    .tooltipText::after {
      content: "";
      position: absolute;
      top: 100%;
      left: 50%;
      margin-left: -5px;
      border-width: 5px;
      border-style: solid;
      border-color: #161b22 transparent transparent transparent;
    }
    .helpTooltip:hover .tooltipText, .helpIcon:focus + .tooltipText {
      visibility: visible;
      opacity: 1;
    }

    /* --- Mobile Responsiveness --- */
    @media (max-width: 600px) {
      body {
        padding: 0.5rem; /* Reduce padding on smaller screens */
      }

      .container {
        padding: 1.5rem; /* Adjust container padding */
        border-radius: 12px; /* Slightly smaller border-radius */
      }

      h1 {
        font-size: 1.5rem; /* Smaller heading for mobile */
        margin-bottom: 1.5rem;
      }

      form {
        gap: 0.75rem; /* Reduce gap between form elements */
      }

      input[type="text"], button {
        padding: 0.7rem; /* Slightly smaller padding for inputs and buttons */
        font-size: 0.95rem; /* Adjust font size */
      }

      .speedControl {
        flex-direction: column; /* Stack speed control elements vertically */
        align-items: stretch; /* Stretch items to fill width */
        gap: 0.75rem; /* Adjust gap */
        padding: 0.5rem 0.75rem; /* Adjust padding */
      }

      .speedControl input[type="range"] {
        width: 100%; /* Make range slider take full width */
      }

      .speedControl input[type="number"] {
        width: 100%; /* Make number input take full width */
        max-width: 100px; /* Limit max width for number input */
        margin: 0 auto; /* Center the number input */
      }

      .tooltipText {
        width: 180px; /* Smaller tooltip width */
        padding: 0.6rem;
        font-size: 0.8rem;
        bottom: 120%; /* Adjust tooltip position */
      }

      .divider {
        margin: 1.5rem 0; /* Adjust divider margin */
      }
    }

    @media (max-width: 400px) {
      .container {
        padding: 1rem;
      }
      h1 {
        font-size: 1.3rem;
      }
      .tooltipText {
        width: 160px;
        font-size: 0.75rem;
      }
    }
  </style>
</head>
<body>
  <main class="container">
    <h1>Morse Code Translator</h1>
    <form id="translateForm" action="/send" method="POST">
      <div class="formHeader">
        <label for="text">Enter your message:</label>
        <div class="helpTooltip">
          <span class="helpIcon" tabindex="0">?</span>
          <span class="tooltipText">Enter text here to translate and send it as Morse code.</span>
        </div>
      </div>
      <input type="text" name="text" id="text" required autofocus>
      <button type="submit">Translate</button>
    </form>
    <div class="divider"></div>
    <form id="speedForm" action="/setSpeed" method="POST">
      <div class="formHeader">
        <label for="speed">Speed:</label>
        <div class="helpTooltip">
          <span class="helpIcon" tabindex="0">?</span>
          <span class="tooltipText">Adjust the transmission speed. The value is sent to the device to control the rate of the Morse code output.</span>
        </div>
      </div>
      <div class="speedControl">
        <input type="range" id="speed" min="1" max="500" value="100" oninput="document.getElementById('speedVal').value = this.value">
        <input type="number" id="speedVal" name="speed" min="1" max="500" value="100" oninput="document.getElementById('speed').value = this.value">
      </div>
      <button type="submit">Set Speed</button>
    </form>
  </main>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleSetSpeed() {
  if (server.hasArg("speed")) {
    int newSpeed = server.arg("speed").toInt();
    if (newSpeed > 0) speedMultiplier = 100.0 / newSpeed;
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Web Speed Input:", TFT_WIDTH / 2, 30);
    tft.setTextSize(2);
    tft.drawString(String(newSpeed) + "%", TFT_WIDTH / 2, TFT_HEIGHT / 2);
    delay(2000 * speedMultiplier);
    mode = start;
    showHomepage();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSend() {
  if (server.hasArg("text")) {
    String webInput = server.arg("text");
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Web Input", TFT_WIDTH / 2, 30);
    delay(2000 * speedMultiplier);
    String inputText = webInput;
    inputText.toLowerCase();
    String cleanedString = "";
    for (size_t i = 0; i < inputText.length(); i++) {
      char currentChar = inputText.charAt(i);
      if (isalnum(currentChar) || currentChar == ' ') {
        cleanedString += currentChar;
      }
    }
    translateToMorse(cleanedString.c_str());
    delay(2000 * speedMultiplier);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1.5);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Translated!", TFT_WIDTH / 2, TFT_HEIGHT / 2);
    delay(2000 * speedMultiplier);
    mode = start;
    showHomepage();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  pinMode(pinButton, INPUT_PULLUP);
  pinMode(pinLed, OUTPUT);
  pinMode(pinShift, INPUT_PULLUP);
  pinMode(pinEnter, INPUT_PULLUP);
  ledcSetup(buzChan, buzFreq, buzRes);
  ledcAttachPin(pinBuzz, buzChan);
  ledcWrite(buzChan, 0);
  tft.init();
  tft.setRotation(2);
  showHomepage();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println(WiFi.localIP());
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);
  tft.println("Web:");
  tft.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/setSpeed", HTTP_POST, handleSetSpeed);
  server.begin();
}

void loop() {
  server.handleClient();
  switch (mode) {
    case start: homepageLoop(); break;
    case input: morseLoop(); break;
    case output: keyboardLoop(); break;
  }
}

void showHomepage() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 20);
  tft.print("Select Mode:");
  String items[2] = {"Morse Input", "Keyboard"};
  for (int i = 0; i < 2; i++) {
    tft.setCursor(20, 50 + i * 20);
    tft.setTextColor(i == menuCursor ? TFT_GREEN : TFT_WHITE);
    if (i == menuCursor) tft.print("> ");
    tft.print(items[i]);
  }
}

void homepageLoop() {
  if (millis() - lastTouch > dbTime) {
    int y = analogRead(pinJoyY);
    if (y < centerVal - deadzoneVal) {
      menuCursor = min(1, menuCursor + 1);
      showHomepage();
      lastTouch = millis();
    } else if (y > centerVal + deadzoneVal) {
      menuCursor = max(0, menuCursor - 1);
      showHomepage();
      lastTouch = millis();
    }
    if (touchRead(pinSelect) < 30) {
      mode = (menuCursor == 0 ? input : output);
      initMode();
      lastTouch = millis();
    }
  }
}

void initMode() {
  tft.fillScreen(TFT_BLACK);
  if (mode == input) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 10);
    tft.print("Morse Input");
    decodedMessage = "";
    currentSymbol = "";
    lastButtonState = HIGH;
    lastChangeTime = millis();
    drawMorseMessage();
  } else {
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 10);
    tft.print("Keyboard Mode");
    typed = "";
    cursorX = cursorY = 0;
    drawKeyboard();
    drawTextArea();
  }
}

void morseLoop() {
  unsigned long now = millis();
  bool btn = digitalRead(pinButton);
  if (btn != lastButtonState && now - lastChangeTime > 50) {
    if (btn == LOW) handlePress(now);
    else handleRelease(now);
    lastChangeTime = now;
    lastButtonState = btn;
  }
  if (btn == HIGH && currentSymbol.length() > 0) {
    unsigned long gap = now - lastChangeTime;
    if (gap >= letterGapThresh) {
      endLetter();
    }
  }
  if (touchRead(pinEnter) < 30) {
    if (currentSymbol.length() > 0) {
      endLetter();
    }
    decodedMessage += ' ';
    drawMorseMessage();
    delay(300);
  }
  if (touchRead(pinShift) < 30) {
    mode = start;
    showHomepage();
    delay(300);
  }
  if (touchRead(pinBackspace) < 30) {
    if (decodedMessage.length() > 0) {
      decodedMessage.remove(decodedMessage.length() - 1);
      drawMorseMessage();
      delay(300);
    }
  }
}

void handlePress(unsigned long t) {
  digitalWrite(pinLed, HIGH);
  ledcWrite(buzChan, 128);
}

void handleRelease(unsigned long t) {
  unsigned long d = t - lastChangeTime;
  digitalWrite(pinLed, LOW);
  ledcWrite(buzChan, 0);
  currentSymbol += (d < dotThresh) ? '.' : '-';
}

void endLetter() {
  decodedMessage += decodeMorse(currentSymbol);
  drawMorseMessage();
  currentSymbol = "";
}

char decodeMorse(const String& s) {
  for (int i = 0; i < 36; i++) if (morseCodeTable[i] == s) return letters[i];
  return ' ';
}

void drawMorseMessage() {
  tft.fillRect(0, 40, TFT_WIDTH, TFT_HEIGHT - 40, TFT_BLACK);
  tft.setCursor(5, 50);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.print(decodedMessage);
  tft.setCursor(5, 100);
  tft.setTextColor(TFT_YELLOW);
  tft.print(currentSymbol);
}

void keyboardLoop() {
  if (millis() - lastTouch > dbTime) {
    int x = analogRead(pinJoyX), y = analogRead(pinJoyY);
    if (x < centerVal - deadzoneVal) {
      cursorX = min(keyCols - 1, cursorX + 1);
      drawKeyboard();
      lastTouch = millis();
    } else if (x > centerVal + deadzoneVal) {
      cursorX = max(0, cursorX - 1);
      drawKeyboard();
      lastTouch = millis();
    }
    if (y < centerVal - deadzoneVal) {
      cursorY = min(keyRows - 1, cursorY + 1);
      drawKeyboard();
      lastTouch = millis();
    } else if (y > centerVal + deadzoneVal) {
      cursorY = max(0, cursorY - 1);
      drawKeyboard();
      lastTouch = millis();
    }
  }
  if (millis() - lastTouch > dbTime) {
    if (touchRead(pinSelect) < 30) {
      selectKey();
      drawTextArea();
      lastTouch = millis();
    } else if (touchRead(pinBackspace) < 30) {
      if (typed.length()) typed.remove(typed.length() - 1);
      drawTextArea();
      lastTouch = millis();
    } else if (touchRead(pinEnter) < 30) {
      typed += '\n';
      drawTextArea();
      lastTouch = millis();
    }
  }
  if (touchRead(pinShift) < 30) {
    mode = start;
    showHomepage();
    delay(300);
  }
  if (typed.endsWith("\n")) {
    translateToMorse(typed.c_str());
    delay(2000 * speedMultiplier);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1.5);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Translated!", TFT_WIDTH / 2, TFT_HEIGHT / 2);
    delay(2000 * speedMultiplier);
    mode = start;
    showHomepage();
  }
}

void selectKey() {
  char c = keyboard[cursorY][cursorX];
  if (c == '<') {
    if (typed.length()) typed.remove(typed.length() - 1);
  } else if (c == '#') {
    typed += '\n';
  } else if (c == '_') {
    typed += ' ';
  } else {
    typed += (char)tolower(c);
  }
}

void drawKeyboard() {
  tft.fillRect(0, 40, TFT_WIDTH, 88, TFT_BLACK);
  for (int y = 0; y < keyRows; y++) {
    for (int x = 0; x < keyCols; x++) {
      int xpos = x * 12, ypos = y * 16 + 40;
      bool sel = (x == cursorX && y == cursorY);
      tft.fillRect(xpos, ypos, 12, 16, sel ? TFT_BLUE : TFT_DARKGREY);
      char c = keyboard[y][x];
      if (c != '<' && c != '#' && c != '_') c = tolower(c);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(1);
      tft.setCursor(xpos + 2, ypos + 2);
      tft.print(c);
    }
  }
}

void drawTextArea() {
  tft.fillRect(0, 0, TFT_WIDTH, 40, TFT_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(TFT_GREEN);
  int ln = typed.lastIndexOf('\n');
  String s = (ln >= 0) ? typed.substring(ln + 1) : typed;
  tft.print(s);
}

void translateToMorse(const char* inputText) {
  for (int i = 0; i < strlen(inputText); i++) {
    char c = toupper(inputText[i]);
    if (c == ' ') {
      showTranslationUI("[SPACE]", " ");
      delay(wordGapThresh * speedMultiplier);
      continue;
    }
    String morse = getMorseCode(c);
    if (morse.length()) {
      showTranslationUI(String(c), morse);
      for (int j = 0; j < morse.length(); j++) blinkSymbol(morse[j]);
      delay(symbolGapThresh * speedMultiplier);
    }
  }
}

String getMorseCode(char c) {
  for (int i = 0; i < 36; i++) if (letters[i] == c) return morseCodeTable[i];
  return "";
}

void showTranslationUI(String letter, String morse) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("Char:", 5, 5);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(letter, 5, 30);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Morse:", 70, 5);
  tft.setTextColor(TFT_GREEN);
  tft.drawString(morse, 70, 30);
}

void blinkSymbol(char symbol) {
  int duration = (symbol == '.') ? dotThresh : (dotThresh * 3);
  duration *= speedMultiplier;
  uint16_t color = (symbol == '.') ? TFT_WHITE : TFT_RED;
  tft.fillRect(0, 70, TFT_WIDTH, 58, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(5);
  tft.setTextColor(color);
  tft.drawString(String(symbol), TFT_WIDTH / 2, 100);
  buzzerOn();
  digitalWrite(pinLed, HIGH);
  delay(duration);
  buzzerOff();
  digitalWrite(pinLed, LOW);
  tft.fillRect(0, 70, TFT_WIDTH, 58, TFT_BLACK);
  delay(dotThresh * speedMultiplier);
}

void buzzerOn() {
  ledcWriteTone(buzChan, 1000);
  ledcWrite(buzChan, buzVol);
}

void buzzerOff() {
  ledcWrite(buzChan, 0);
}
