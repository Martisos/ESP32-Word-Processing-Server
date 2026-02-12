from flask import Flask, request, jsonify
import os
import re
import pdfplumber
import whisper
import json
import platform
import requests
import ffmpeg



#certificate error fix (inside venv):
#python -m pip install --upgrade pip
#python -m pip install requests certifi



app = Flask(__name__)
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# ----------------- Utility functions -----------------
def clear():
    system_name = platform.system()
    if system_name == "Windows":
        os.system("cls")
    else:
        os.system("clear")

def extract_words(text):
    words = re.findall(r"[a-zA-ZąćęłńóśżźĄĆĘŁŃÓŚŻŹ]+", text)
    return words

def words_from_txt(path):
    with open(path, "r", encoding="utf-8-sig") as f:
        text = f.read()
    return extract_words(text)

def words_from_pdf(path):
    text = ""
    with pdfplumber.open(path) as pdf:
        for page in pdf.pages:
            page_text = page.extract_text()
            if page_text:
                text += page_text + " "
    return extract_words(text)

def words_from_mp3(path):
    model = whisper.load_model("medium")  # tiny / base / small / medium / large
    result = model.transcribe(path, language="pl")
    text = result["text"]
    return extract_words(text)

def wordInSecond(word):
    if len(word) == 1:
        return 0.1
    else:
        return round(len(word) * 0.058, 3)

def array_to_json(words, output_path="words.json"):
    dict_word = {}
    for counter, word in enumerate(words):
        dict_word[counter] = {word: wordInSecond(word)}
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(dict_word, f, ensure_ascii=False, indent=2)
    return output_path

def send_to_esp32(json_file_path, esp32_ip):
    try:
        esp32_url = f"http://{esp32_ip}/upload"
        with open(json_file_path, "rb") as f:
            r = requests.post(esp32_url, data=f.read(), timeout=5)
        return {
            "status": r.status_code,
            "response": r.text
        }
    except Exception as e:
        return {"error": str(e)}

# ----------------- Flask routes -----------------
@app.route("/")
def index():
    return """
    <html>
        <head>
            <title>File Upload to ESP32</title>
            <link rel="stylesheet" href="{{ url_for('static', filename='style.css') }}">

        </head>
        <body>
            <div class="container">
                <h1>Upload TXT, PDF, or MP3</h1>
                <form method="post" action="/upload_file" enctype="multipart/form-data">
                    <input type="file" name="file" required><br>
                    <input type="text" name="ip" placeholder="ESP32 IP (e.g., 192.168.1.50)" required><br>
                    <input type="submit" value="Upload & Send">
                </form>
            </div>
        </body>
    </html>
    """

@app.route("/upload_file", methods=["POST"])
def upload_file():
    if "file" not in request.files:
        return jsonify({"error": "No file part"}), 400

    file = request.files["file"]
    if file.filename == "":
        return jsonify({"error": "No selected file"}), 400

    esp32_ip = request.form.get("ip")
    if not esp32_ip:
        return jsonify({"error": "ESP32 IP not provided"}), 400

    file_path = os.path.join(UPLOAD_FOLDER, file.filename)
    file.save(file_path)

    try:
        # Convert file to words array
        if file.filename.endswith(".txt"):
            words = words_from_txt(file_path)
        elif file.filename.endswith(".pdf"):
            words = words_from_pdf(file_path)
        elif file.filename.endswith(".mp3"):
            words = words_from_mp3(file_path)
        else:
            return jsonify({"error": "Unsupported file type"}), 400

        # Convert to JSON
        json_path = array_to_json(words)

        # Send to ESP32
        esp32_result = send_to_esp32(json_path, esp32_ip)

        return jsonify({
            "message": "File processed and sent to ESP32",
            "esp32_ip": esp32_ip,
            "esp32_result": esp32_result
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ----------------- Run server -----------------
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
