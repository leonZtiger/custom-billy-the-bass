// Custom Billy the Bass Code
// 1. Ensure the SD card only contains .wav files.
// 2. WAV files must be 8-bit, mono, with a maximum sample rate of 16,000 Hz.
// 3. Designed for ESP32-WROOM, though delays and settings might need adjustment for other boards.
// 4. Set tasks to run on core 0 and clock speed to 240 MHz.
// 5. Follow the wiring diagrams in the README for proper setup.

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <vector>
#include <string>

// Sensitivity thresholds
#define TALK_SENS 135
#define HEAD_SENS 170

// Buffer settings
#define BUFFER_SIZE 8192

// Pin assignments
#define DAC_PIN 26
#define TALK_MOTOR_PIN 21
#define HEAD_MOTOR_PIN 13
#define SPEAKER_ACTIVATE 15
#define SENSOR_PIN 35
#define BUTTON_PIN 33

// Global variables
std::vector<std::string> audio_files;
char audio_buffers[2][BUFFER_SIZE];
bool talk_track[2][BUFFER_SIZE];
bool currentBufferIndex = false;
SemaphoreHandle_t parseMutex;
bool parse = false;
bool terminatePlayer = false;

// Function to retrieve .wav files from the SD card
std::vector<std::string> getAudioFilenames(fs::FS& sd) {
    std::vector<std::string> files;
    File root = sd.open("/");
    File file = root.openNextFile();

    while (file) {
        if (!file.isDirectory()) {
            files.push_back(std::string("/") + file.name());
        }
        file = root.openNextFile();
    }
    return files;
}

// Task to play buffered audio
void bufferPlayerTask(void* frameDelay) {
    while (!terminatePlayer) {
        xSemaphoreTake(parseMutex, portMAX_DELAY);
        if (parse) {
            parse = false;
            for (int i = 0; i < BUFFER_SIZE; i++) {
                dacWrite(DAC_PIN, (uint8_t)(audio_buffers[currentBufferIndex][i]));
                digitalWrite(TALK_MOTOR_PIN, talk_track[currentBufferIndex][i]);
                delayMicroseconds(*((int*)frameDelay));
            }
        }
        xSemaphoreGive(parseMutex);
    }
    vTaskDelete(NULL);
}

// Function to play a .wav file
void playAudioFromFile(const std::string& path, fs::FS& sd) {
    File file = sd.open(path.c_str(), FILE_READ);
    if (!file) {
        Serial.println("Failed to open file: " + path);
        return;
    }

    // Parse WAV header
    uint16_t numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;

    char buffer[22];
    file.readBytes(buffer, sizeof(buffer));
    file.readBytes((char*)&numChannels, 2);
    file.readBytes((char*)&sampleRate, 4);
    file.readBytes(buffer, 6); // Skip bytes
    file.readBytes((char*)&bitsPerSample, 2);

    // Check audio format
    if (bitsPerSample > 8 || numChannels != 1) {
        Serial.println("Invalid WAV file format (must be 8-bit mono).");
        file.close();
        return;
    }

    uint32_t frameDelay = (1000000.0f / sampleRate) - 18;
    file.readBytes(audio_buffers[currentBufferIndex], BUFFER_SIZE);

    terminatePlayer = false;
    TaskHandle_t bufferTask;
    xTaskCreatePinnedToCore(bufferPlayerTask, "BufferPlayer", BUFFER_SIZE * 2 + 10000, &frameDelay, 1, &bufferTask, 1);

    while (file.available()) {
        parse = true;
        xSemaphoreGive(parseMutex);

        file.readBytes(audio_buffers[!currentBufferIndex], BUFFER_SIZE);

        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (audio_buffers[!currentBufferIndex][i] > TALK_SENS) {
                for (int j = max(i - 300, 0); j < min(i + 500, BUFFER_SIZE); j++) {
                    talk_track[!currentBufferIndex][j] = true;
                }
                i += 500;
            } else {
                talk_track[!currentBufferIndex][i] = false;
            }
        }

        digitalWrite(HEAD_MOTOR_PIN, random(0, 10) == 0 ? HIGH : LOW);
        xSemaphoreTake(parseMutex, 1000);
        currentBufferIndex = !currentBufferIndex;
    }

    terminatePlayer = true;
    file.close();
}

// Setup function
void setup() {
    pinMode(TALK_MOTOR_PIN, OUTPUT);
    pinMode(SPEAKER_ACTIVATE, OUTPUT);
    pinMode(HEAD_MOTOR_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(DAC_PIN, OUTPUT);

    digitalWrite(SPEAKER_ACTIVATE, LOW);
    Serial.begin(115200);

    if (!SD.begin()) {
        Serial.println("SD card mount failed!");
        return;
    }

    audio_files = getAudioFilenames(SD);
    Serial.printf("Found %d files.\n", audio_files.size());
    parseMutex = xSemaphoreCreateMutex();
}

// Play a random audio file
void aliveFish() {
    int soundIndex = random(0, audio_files.size());
    Serial.println(audio_files[soundIndex].c_str());

    digitalWrite(SPEAKER_ACTIVATE, HIGH);
    playAudioFromFile(audio_files[soundIndex], SD);

    digitalWrite(SPEAKER_ACTIVATE, LOW);
    digitalWrite(HEAD_MOTOR_PIN, LOW);
    digitalWrite(TALK_MOTOR_PIN, LOW);
}

// Main loop
void loop() {
    aliveFish();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
    esp_deep_sleep_start();
}
