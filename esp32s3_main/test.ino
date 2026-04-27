// #include <driver/i2s.h>
// #include <math.h>

// // Audio settings
// #define SAMPLE_BUFFER_SIZE 512
// #define SAMPLE_RATE 16000

// // VAD Settings
// #define VAD_THRESHOLD 1500  // Adjust this based on Serial Plotter observation
// #define ENERGY_SCALING 14    // Your current bit-shift for INMP441

// // XIAO ESP32S3 I2S Pins
// #define I2S_MIC_SERIAL_CLOCK 9     // BCLK/SCK
// #define I2S_MIC_LEFT_RIGHT_CLOCK 8   // WS/LRCL
// #define I2S_MIC_SERIAL_DATA 7      // SD

// // I2S config
// i2s_config_t i2s_config = {
//   .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
//   .sample_rate = SAMPLE_RATE,
//   .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
//   .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//   .communication_format = I2S_COMM_FORMAT_I2S,
//   .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//   .dma_buf_count = 8,
//   .dma_buf_len = 256,
//   .use_apll = false
// };

// i2s_pin_config_t i2s_mic_pins = {
//   .bck_io_num = I2S_MIC_SERIAL_CLOCK,
//   .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
//   .data_out_num = I2S_PIN_NO_CHANGE,
//   .data_in_num = I2S_MIC_SERIAL_DATA
// };

// int32_t raw_samples[SAMPLE_BUFFER_SIZE];

// void setup() {
//   Serial.begin(115200);
//   while(!Serial); 

//   Serial.println("Initializing I2S Mic with VAD...");
//   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
//   i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
//   i2s_zero_dma_buffer(I2S_NUM_0);

//   Serial.println("Format: RMS, Threshold, VAD_State");
// }

// void loop() {
//   size_t bytes_read = 0;
//   i2s_read(I2S_NUM_0, raw_samples, sizeof(raw_samples), &bytes_read, portMAX_DELAY);

//   int samples_read = bytes_read / sizeof(int32_t);
  
//   if (samples_read > 0) {
//     float sum_sq = 0;

//     for(int i = 0; i < samples_read; i++) {
//       // Process sample as you did in your working code
//       int32_t processed_sample = raw_samples[i] >> ENERGY_SCALING; 
      
//       // Accumulate for RMS
//       sum_sq += (float)processed_sample * processed_sample;
//     }

//     float rms = sqrt(sum_sq / samples_read);
//     int vad_active = (rms > VAD_THRESHOLD) ? 1 : 0;

//     // Output for Serial Plotter
//     Serial.print(rms);           // Blue line
//     Serial.print(",");
//     Serial.print(VAD_THRESHOLD); // Red line (Target)
//     Serial.print(",");
//     Serial.println(vad_active * 1000); // Green spikes for detection

//     // Logic for your Pipeline
//     if (vad_active) {
//       /* STAGE 2 CLASSIFIER TRIGGER:
//          If RMS > Threshold, this is where we will eventually 
//          pass the raw_samples to the Edge Impulse model to check 
//          for PANIC vs WAKEWORD vs CONVERSATION.
//       */
//     }
//   }
// }





#include <driver/i2s.h>
#include <math.h>

#define SAMPLE_RATE 16000
#define FRAME_SIZE 256

#define VAD_THRESHOLD 1200
#define HANGOVER_FRAMES 30

// XIAO pins
#define I2S_BCLK 9
#define I2S_WS   8
#define I2S_SD   7

int32_t raw_samples[FRAME_SIZE];

bool recording=false;
int silence_count=0;

i2s_config_t i2s_config={
 .mode=(i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
 .sample_rate=SAMPLE_RATE,
 .bits_per_sample=I2S_BITS_PER_SAMPLE_32BIT,
 .channel_format=I2S_CHANNEL_FMT_ONLY_LEFT,
 .communication_format=I2S_COMM_FORMAT_I2S,
 .intr_alloc_flags=ESP_INTR_FLAG_LEVEL1,
 .dma_buf_count=8,
 .dma_buf_len=256,
 .use_apll=false
};

i2s_pin_config_t pins={
 .bck_io_num=I2S_BCLK,
 .ws_io_num=I2S_WS,
 .data_out_num=I2S_PIN_NO_CHANGE,
 .data_in_num=I2S_SD
};

void setup() {

 Serial.begin(921600);

 i2s_driver_install(
   I2S_NUM_0,
   &i2s_config,
   0,
   NULL
 );

 i2s_set_pin(
   I2S_NUM_0,
   &pins
 );
}

void loop(){

 size_t bytes_read=0;

 i2s_read(
   I2S_NUM_0,
   raw_samples,
   sizeof(raw_samples),
   &bytes_read,
   portMAX_DELAY
 );

 int n=bytes_read/4;

 float sum=0;
 int16_t audio[FRAME_SIZE];

 for(int i=0;i<n;i++){
    audio[i]=raw_samples[i]>>16;
    sum += audio[i]*audio[i];
 }

 float rms=sqrt(sum/n);

 bool speech=(rms>VAD_THRESHOLD);

 // trigger
 if(speech && !recording){
    recording=true;
    silence_count=0;

    // start marker
    Serial.println("<START>");
 }

 if(recording){

    // send raw frame
    for(int i=0;i<n;i++){
       Serial.println(audio[i]);
    }

    if(speech){
       silence_count=0;
    }
    else{
       silence_count++;
    }

    if(silence_count>HANGOVER_FRAMES){
       Serial.println("<END>");
       recording=false;
    }
 }
}