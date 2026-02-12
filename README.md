# ESP32-Word-Processing-Server

**Local AI-powered word streaming system for ESP32**

Extracts words from **TXT / PDF / MP3 (Whisper)**, converts them into timed JSON, and streams them to an **ESP32**, which displays them sequentially on an **OLED screen** with hardware-controlled speed.

---

## 📌 Overview

This project consists of two main parts:

### 1️⃣ Flask Server (Python)
- Accepts TXT, PDF, or MP3 files
- Extracts words from text
- Uses OpenAI Whisper for MP3 transcription
- Converts words into timed JSON format
- Sends processed data to ESP32 via HTTP

### 2️⃣ ESP32 Client
- Hosts an HTTP server
- Receives JSON file
- Stores it in LittleFS
- Parses words into a dynamic vector
- Displays words sequentially on an OLED screen
- Playback speed controlled via potentiometer
- Restart controlled via button

---

## 🏗 System Architecture

```
User 
   ↓
Flask Web App 
   ↓
Word Extraction / Whisper
   ↓
JSON Generation
   ↓
ESP32 (HTTP POST /upload)
   ↓
OLED Display + Speed Control
```

---

# 🖥 Flask Server

## 🐍 Python Requirements

Install required libraries:

```bash
pip install flask pdfplumber openai-whisper requests ffmpeg-python
```

### Libraries Used

- **flask** – web server & file upload handling  
- **pdfplumber** – PDF text extraction  
- **openai-whisper** – MP3 transcription  
- **requests** – sending JSON to ESP32  
- **ffmpeg-python** – audio backend for Whisper  

---

## ⚙ System Requirement

Whisper requires **FFmpeg installed on your system**.

Check if installed:

```bash
ffmpeg -version
```

### Install FFmpeg

**Windows**  
Download from: https://ffmpeg.org/download.html  
Add FFmpeg to system PATH.

**Linux**
```bash
sudo apt install ffmpeg
```

**macOS**
```bash
brew install ffmpeg
```

---

## 🚀 Run Flask Server

```bash
python app.py
```

By default Flask runs on:

```
http://127.0.0.1:5000
```

Open in browser:

```
http://127.0.0.1:5000
```

---

## 📄 JSON Format Sent to ESP32

Example:

```json
{
  "0": {"Hello": 0.29},
  "1": {"world": 0.29},
  "2": {"example": 0.406}
}
```

Each word contains a calculated display duration:

```
duration = len(word) * 0.058
```

---

# 🔌 ESP32 Firmware

## Features

- WiFi connection
- HTTP server (`/upload`)
- LittleFS storage
- ArduinoJson parsing
- OLED 128x64 display (SSD1306)
- UTF-8 Polish character support
- Potentiometer speed control
- Button restart
- Dynamic word vector storage

---

## 🧰 Required Arduino Libraries

Install via Arduino IDE Library Manager:

- WiFi
- WebServer
- LittleFS
- ArduinoJson
- U8g2
- Wire

---

## 🔧 Hardware Setup

| Component         | GPIO |
|------------------|------|
| OLED (I2C) SDA   | 21   |
| OLED (I2C) SCL   | 22   |
| Potentiometer    | 34   |
| Button           | 18   |

OLED: **SSD1306 128x64**

---

## 📶 WiFi Configuration

Inside ESP32 code:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
```

After boot, ESP32 prints IP in Serial Monitor:

```
WiFi OK
192.168.1.xxx
```

Use this IP in Flask web interface.

---

## 🔄 How It Works

### Upload Flow

1. User uploads file via web form
2. Flask:
   - Extracts words
   - Generates JSON
   - Sends to ESP32 `/upload`
3. ESP32:
   - Saves JSON to LittleFS
   - Parses into vector
   - Displays words sequentially

---

## 🎛 Playback Logic (ESP32)

Display delay is calculated as:

```
delay = (750 * word_weight * 30) / potentiometer_value
```

Constrained between:

```
50 ms – 1000 ms
```

### Controls

- Potentiometer → Controls reading speed
- Button → Restart from first word

---

# 🧠 AI Component

MP3 files are transcribed using:

```
Whisper model: medium
Language: Polish (pl)
```

You can change model size:

```python
whisper.load_model("tiny")
whisper.load_model("base")
whisper.load_model("small")
whisper.load_model("medium")
whisper.load_model("large")
```

---

# 📂 Project Structure

```
ESP32-Word-Processing-Server/
│
├── flask_app/
│   └── app.py
│
├── esp32_firmware/
│   └── main.ino
│
├── uploads/
├── words.json
└── README.md
```

---

# ⚠ Limitations

- Whisper model is CPU-intensive
- JSON size limited by ESP32 memory
- Large PDFs may require optimization
- MP3 transcription may take time

---

# 🔒 Local-Only Design

This system works entirely in local network:

- No cloud processing
- No external APIs
- Direct LAN communication

---

# 📜 License

MIT License
