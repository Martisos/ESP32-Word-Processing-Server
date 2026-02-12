#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define UPLOAD_BUF_SIZE 4096
uint8_t buf[UPLOAD_BUF_SIZE];

#define OLED_RESET -1  
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int potValue = 0;
const int potPin = 34;
const int buttonPin = 18;  

int lastButtonState = HIGH;
bool ended = false;

const char* ssid = ""; // Your wifi name
const char* password = ""; // Your wifi password

WebServer server(80);
File uploadFile;

// ===== Struktura słowa =====
struct Word {
  String word;
  float weight;
};

std::vector<Word> words; // dynamiczna tablica

void readPotentiometr(){
  potValue = analogRead(potPin);
  potValue = potValue / 100;  

}
// ===== ODBIÓR PLIKU =====
volatile bool wordsUpdated = false;

void handleUpload() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No body received");
    return;
  }

  String body = server.arg("plain");

  File f = LittleFS.open("/words.json", FILE_WRITE);
  if (!f) {
    server.send(500, "text/plain", "Failed to open file");
    return;
  }

  f.print(body);
  f.close();

  parseJson();   // fills words vector
  wordsUpdated = true;  // flag that new words are ready

  server.send(200, "text/plain", "OK");
}

// ===== PARSOWANIE JSON DO VECTOR =====
void parseJson() 
{
  File f = LittleFS.open("/words.json", "r");
  if (!f) {
      Serial.println("Nie znaleziono pliku words.json");
      return;
  }

  words.clear();

  String jsonString = f.readString();
  f.close();

  DynamicJsonDocument doc(16384); 
  DeserializationError err = deserializeJson(doc, jsonString);

  if(err) {
      Serial.print("Błąd parsowania JSON: ");
      Serial.println(err.c_str());
      return;  // stop here
  }

  JsonObject root = doc.as<JsonObject>();
  for (JsonPair kv : root) {
      JsonObject inner = kv.value().as<JsonObject>();
      for (JsonPair w : inner) {
          Word wordEntry;
          wordEntry.word = String(w.key().c_str());  // UTF-8
          wordEntry.weight = w.value().as<float>();
          words.push_back(wordEntry);
      }
  }

  Serial.print("Parsowanie zakończone. Liczba słów: ");
  Serial.println(words.size());
}
// ===== SETUP =====
void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  Wire.begin(21, 22); 
  Serial.begin(115200);
  u8g2.begin();                 
  u8g2.enableUTF8Print();       
  u8g2.setFont(u8g2_font_unifont_t_polish);
  u8g2.setFontDirection(0);     // left → right
  delay(2000); 


  // ---- LittleFS ----
  if (!LittleFS.begin(true)) {
    Serial.println("Błąd LittleFS");
    while (1);
  }
  Serial.println("LittleFS OK");

  // ---- WiFi ----
  WiFi.begin(ssid, password);
  Serial.print("Łączenie WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");
  Serial.println(WiFi.localIP());

  // ---- Serwer HTTP ----
  server.on("/upload", HTTP_POST, handleUpload);

  server.begin();
  Serial.println("HTTP server OK");

  // jeśli plik już istnieje, wczytujemy go
  if (LittleFS.exists("/words.json")) parseJson();
}

// ===== LOOP =====
void loop() {
  server.handleClient();
  
  bool currentButtonState = digitalRead(buttonPin);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    ended = false;
  }
  lastButtonState = currentButtonState;
  if (wordsUpdated) 
  {
    wordsUpdated = false;
    ended = false;
  }
  if(ended == false)
  {
    for (size_t i = 0; i < words.size(); i++) {
      Word &w = words[i];
      float percent = ((((i+1)*100)/words.size()));
      while (true) {
        server.handleClient();
        readPotentiometr();

        currentButtonState = digitalRead(buttonPin);
        if (lastButtonState == HIGH && currentButtonState == LOW) {
          i = -1;
          break;   // exit immediately on click
        }
        lastButtonState = currentButtonState;

        // restart loop if words were updated
        if (wordsUpdated) {
          wordsUpdated = false;
          i = 0;          // restart from first word
          break;
        }

        if (potValue == 0) {
          delay(10);
          continue;
        }

        int d = (750 * w.weight * 30)/ potValue;
        d = constrain(d, 50, 1000);

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_unifont_t_polish);
        u8g2.setCursor(28,32);
        u8g2.print(w.word);
        u8g2.setCursor(((SCREEN_WIDTH/2)-18),64);
        u8g2.print(percent);
        u8g2.print('%');
        u8g2.sendBuffer();
        delay(d);
        break;  // next word
      }
    }
    ended = true;
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  }
}