// important!
// 1. Make sure that the sd-card only contains .wav files.
// 2. The wav files need to be 8-bit, mono with a sample rate of 16000 max
// 3. This code is meant to run on the esp32-wroomer, it can be ran on other platforms but delays and such might need changing. 
// 4. Set Event to run on core 0.
// 5. Set the clock speed to be 240mhz.
// 6. Connect the sd-card accordingly to the readme pictures

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <vector>
#include <string>

#define TALK_SENS 135
#define HEAD_SENS 170

#define BUFFER_SIZE 4096 * 2

#define DAC_PIN 26
#define TALK_MOTOR_PIN 21
#define HEAD_MOTOR_PIN 13
#define SPEAKER_ACTIVATE 15
#define SENSOR_PIN 35
#define BUTTON_PIN 33

std::vector<std::string> audio_files;

char audio_buffers[2][BUFFER_SIZE];
bool talk_track[2][BUFFER_SIZE];
// buffer_index is used for playback and !buffer_index for filling with new data
bool buffer_index = false;
// mutex for syncing chunk loading
SemaphoreHandle_t parse_mutex;
bool parse = false;
bool killPlayer = false;

std::vector<std::string> get_audio_filenames(fs::FS& sd) {
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

void buffer_player_task(void* frame_delay) {

  while (!killPlayer) {

    xSemaphoreTake(parse_mutex, portMAX_DELAY);

    if (parse) {
      parse = false;
      // play the buffer
      for (int i = 0; i < BUFFER_SIZE; i++) {

        dacWrite(DAC_PIN, (uint8_t)(audio_buffers[buffer_index][i]));
        digitalWrite(TALK_MOTOR_PIN, talk_track[buffer_index][i]);

        delayMicroseconds(*((int*)frame_delay));
      }
    }
    xSemaphoreGive(parse_mutex);
  }
  vTaskDelete(NULL);
}


void play_audio_from_file(std::string& path, fs::FS& sd_ref) {

  File file = sd_ref.open(path.c_str(), FILE_READ);

  if (!file) {
    Serial.println("failed to open file: ");
    Serial.print(path.c_str());
    return;
  }

  // load header
  uint16_t numChannels = 0;
  uint32_t sampleRate = 0;
  uint16_t bitsPerSample = 0;

  char junk[22];
  file.readBytes(junk, sizeof(junk));

  char channel_chunk[2];
  file.readBytes(channel_chunk, sizeof(channel_chunk));
  numChannels = ((uint16_t)channel_chunk[1]) << 8 | (uint16_t)channel_chunk[0];

  char sampleRate_chunk[4];
  file.readBytes(sampleRate_chunk, sizeof(sampleRate_chunk));
  sampleRate = ((uint32_t)sampleRate_chunk[3]) << 24 | ((uint32_t)sampleRate_chunk[2]) << 16 | ((uint32_t)sampleRate_chunk[1]) << 8 | (uint32_t)sampleRate_chunk[0];

  char junk2[6];
  file.readBytes(junk2, sizeof(junk2));

  char bitsPerSample_chunk[2];
  file.readBytes(bitsPerSample_chunk, sizeof(bitsPerSample_chunk));
  bitsPerSample = (uint16_t)bitsPerSample_chunk[1] << 8 | (uint16_t)bitsPerSample_chunk[0];

  file.readBytes(nullptr, sizeof(char) * 8);

  Serial.println(numChannels);
  Serial.println(sampleRate);
  Serial.println(bitsPerSample);

  uint32_t frame_delay = (1000000.0f / (sampleRate) / numChannels) - 18;

  if (bitsPerSample > 8) {
    Serial.println("Too high resolution, must be unsigned 8-bit");
    Serial.print(path.c_str());
    return;
  }

  file.readBytes(audio_buffers[buffer_index], sizeof(audio_buffers[buffer_index]));

  killPlayer = false;
  
  TaskHandle_t bufferTask;
  // launch player on second core
  xTaskCreatePinnedToCore(buffer_player_task, "buffer_player", BUFFER_SIZE * 2 + 10000, (void*)&frame_delay, 1, &bufferTask, 1);
  
  while (file.available()) {
    //sync parser task
    parse = true;
    xSemaphoreGive(parse_mutex);
    file.readBytes(audio_buffers[!buffer_index], sizeof(audio_buffers[buffer_index]));
    
    // create talk animation
    for (int i = 0; i < BUFFER_SIZE; i++) {
      
      if (audio_buffers[!buffer_index][i] > TALK_SENS) {
        // open mouth 300 frames before and 500, theres a small delay and this make it seamless
        for (int j = max(i - 300, 0); j < min(i + 500, BUFFER_SIZE); j++) {
          talk_track[!buffer_index][j] = 1;
        }
        i += 500;
      } else {
        talk_track[!buffer_index][i] = 0;
      }
    }
    
    // move head randomly
    if (random(0, 10) == 0) {
      digitalWrite(HEAD_MOTOR_PIN, HIGH);
    } else{
      digitalWrite(HEAD_MOTOR_PIN, LOW);
    }
    
    xSemaphoreTake(parse_mutex, 1000);
    buffer_index = !buffer_index;
  }
  killPlayer = true;
  file.close();
}

void setup() {

  pinMode(TALK_MOTOR_PIN, OUTPUT);
  pinMode(SPEAKER_ACTIVATE, OUTPUT);
  pinMode(HEAD_MOTOR_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DAC_PIN, OUTPUT);

  // turn off speakers
  digitalWrite(SPEAKER_ACTIVATE, LOW);

  Serial.begin(115200);

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  audio_files = get_audio_filenames(SD);

  Serial.println("Found ");
  Serial.print(audio_files.size());
  Serial.print(" files.");

  parse_mutex = xSemaphoreCreateMutex();
}

void alive_fish() {
  // get random file
  int sound_index = random(0, audio_files.size());
  Serial.println(audio_files[sound_index].c_str());
  
  // activate speaker
  digitalWrite(SPEAKER_ACTIVATE, HIGH);
  play_audio_from_file(audio_files[sound_index], SD);

  // kill fish
  digitalWrite(SPEAKER_ACTIVATE, LOW);
  digitalWrite(HEAD_MOTOR_PIN, LOW);
  digitalWrite(TALK_MOTOR_PIN, LOW);
}

void loop() {
  alive_fish();
  
  // sleep until button is pressed, this lowers the current consumption and the batteri life span increases
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 0);
  esp_deep_sleep_start();
}
