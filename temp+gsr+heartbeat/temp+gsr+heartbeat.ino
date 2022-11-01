#include <SD.h>
#include <SPI.h>

File myFile;

#define TEMP      1
#define GSR       2
#define HEART     0
#define CS_PIN    53
#define BAUDRATE  57600
#define SD_EN     5
#define LED1      6
#define LED2      7
#define LED3      8
#define LED4      9

//define state
enum OP_STATE{
  INIT = 0,
  READ,
  WRITE,
}state_machine;

// pulse sensor
  int hb_raw = 0, prev_read = 0,obs_ave = 0, count = 0, hb_trig = 0;
  int heartbeat, tmr_rst = 1;
  int ema_count, bpm_ave, prev_bpm;
  long tmr_count;
  const int no_exam = 12, threshold_hb = 25, bpm_exam = 3;
  float ema = 0, bpm, bpm_filter;
  
// brainwave sensor
int   generatedChecksum = 0;
byte  checksum = 0; 
byte  payloadLength = 0;
byte  payloadData[32] = {0};
byte signalquality = 0;//信号质量
byte attention = 0;    //注意力值
byte meditation = 0;   //放松度值

// gsr sensor 
int gsr_in, gsr_raw, gsr_prev, gsr_ave, count_gsr, ema_gsr ;
const int gsr_exam = 7;
int gsr_freq_low,gsr_freq_high, gsr_initial_time, gsr_frequency;
int ave_gsr_freq, total_gsr_freq, gsr_freq_cnt, gsr_max = 0, gsr_min=5000, gsr_amplitude;
int gsr_min_final, gsr_max_final;

// temperature
int temperature_raw;

// sd write control
int sd_allow = 0, sd_flag = 0, sd_newline = 0;

void setup() {
  state_machine = INIT;
  Serial.begin(BAUDRATE);
  pinMode(CS_PIN, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(SD_EN,INPUT);
  pinMode(TEMP,INPUT);
  pinMode(GSR,INPUT);
  pinMode(HEART,INPUT);
//  while(!SD.begin());   // used to check the availability of sd card
}

void loop() {
  pulse_sensor();
  galvanic_skin_resistance();
  temperature();
//  sd_card_write();
//  if (digitalRead(SD_EN)){
//    if(!sd_flag){
//      sd_flag = 1;
//      sd_allow = !sd_allow;
//    }
//  }
//  else sd_flag = 0; 
//  
//  if (sd_allow){
//    sd_newline = 1;
//    digitalWrite(LED1, HIGH);
//    sd_card_write();
//  }
//  else{
//    if(sd_newline) sd_card_newline();
//    digitalWrite(LED1, LOW);
//    sd_newline = 0;
//  }

//Serial.print("temperature = ");
//Serial.println(temperature_raw);
//Serial.print("GSR = ");
//Serial.println(gsr_in);
//Serial.print("BPM = ");
//Serial.println(bpm_filter);
}

/*-----------------------------------
 * WRITE DATA TO SD CARD
-----------------------------------*/
// 
void sd_card_write(){
  myFile = SD.open("data_log.txt", FILE_WRITE);
  if(myFile){
    myFile.print((int)bpm_filter);
    myFile.print(",");
    myFile.print((int)temperature_raw);
    myFile.print(",");
    myFile.print((int)gsr_in);
    myFile.print(",");
    myFile.print((int)gsr_max_final);
    myFile.print(",");
    myFile.print((int)gsr_min_final);
    myFile.print(",");
    myFile.print((int)gsr_amplitude);
    myFile.print(",");
    myFile.print((int)ave_gsr_freq);
    myFile.println(",");
    myFile.close(); // close the file
  }
}

/*-----------------------------------
 * PULSE SENSOR
-----------------------------------*/
// smoothing the data
float ema_cal(int no_obs, int raw_curr, int raw_prev){
  float ema_result;
  float smooth;
  smooth = 2.0 / ((float)no_obs+1.0);
  ema_result = ((float)raw_curr*smooth) + ((float)raw_prev*(1-smooth));
  return ema_result;
}

// data processing
// printing value : heartbeat rate
void pulse_sensor(){
  hb_raw = analogRead(HEART);          //raw data read from sensor
  ema = ema_cal(no_exam, hb_raw, prev_read);         // ema calculation
  if(count<=no_exam)obs_ave = obs_ave + hb_raw;         //calculate ema in smoothing the data
  else{
    count = 0;
    prev_read = obs_ave/no_exam;
    obs_ave = 0;
  }
  count++;
  if(!hb_trig && (hb_raw > (ema+threshold_hb))) hb_trig = 1;
  if(hb_trig && (hb_raw < ema)){
    heartbeat++;
    hb_trig = 0;
  }
  if(tmr_rst){
    tmr_count = millis();
    tmr_rst = 0;
  }
  if(heartbeat>=6){
    bpm = ((float)heartbeat/(float)(millis()-tmr_count))*60000.0;
    
    if(ema_count<=bpm_exam)bpm_ave = bpm_ave + (int)bpm;         //calculate ema in smoothing the data
    else{
      ema_count = 0;
      prev_bpm = (int)(bpm_ave/bpm_exam);
      bpm_ave = 0;
    }
    ema_count++;
    bpm_filter = ema_cal(bpm_exam, bpm, prev_bpm);
    
    tmr_rst = 1;
    heartbeat = 0;
  }
////  debug use
  Serial.print(hb_raw);
  Serial.print(',');
  Serial.print((int)ema);
  Serial.print(',');
  Serial.print((int)ema+25);
  Serial.print(',');
  Serial.print((int)bpm_filter);
  Serial.println();

}


/*------------------------------------------------------
 * GSR Sensor
------------------------------------------------------*/

//data processing
// printing data : gsr_in
void galvanic_skin_resistance(){
  gsr_in = analogRead(GSR);
  ema_gsr = ema_cal(gsr_exam, gsr_in, gsr_prev);         // ema calculation
  if(count_gsr<=gsr_exam)gsr_ave = gsr_ave + gsr_in;         //calculate ema in smoothing the data
  else{
    count_gsr = 0;
    gsr_prev = gsr_ave/gsr_exam;
    gsr_ave = 0;
  }
  count_gsr++;
  /*******************************************/
  if(gsr_in>ema_gsr){
    if(gsr_freq_low){
      gsr_frequency = millis()-gsr_initial_time;
      gsr_freq_low = 0;
      gsr_freq_high = 0;
      gsr_amplitude = gsr_max-gsr_min;
      gsr_max = 0;
      gsr_min = 5000;
    }
    else{
      gsr_freq_high = 1;
      gsr_initial_time=millis();
      if (gsr_max<gsr_in) gsr_max = gsr_in;
      else gsr_max = gsr_max;
    }
  }
  else{
    if(gsr_freq_high){
      gsr_freq_low = 1;
      if (gsr_min>gsr_in) gsr_min = gsr_in;
      else gsr_min = gsr_min;
    }
  }
 /******************************************/
  if(gsr_freq_cnt>=10){
    ave_gsr_freq = total_gsr_freq/10;
    gsr_freq_cnt = 0;
  }
  else {
    gsr_freq_cnt++;
    total_gsr_freq = total_gsr_freq + gsr_frequency;
  }

  if(gsr_max!=0)gsr_max_final = gsr_max;
  if(gsr_min!=5000)gsr_min_final = gsr_min;

////debug use
  Serial.print(gsr_in);
  Serial.print(',');
  Serial.print((int)ema_gsr);
  Serial.print(',');
  Serial.print(gsr_max_final);
  Serial.print(',');
  Serial.print(gsr_min_final);
  Serial.print(',');
  Serial.print(gsr_amplitude);
//  Serial.print(',');
//  Serial.print(ave_gsr_freq);
  Serial.println();
}

/*------------------------------------------------------
 * Temperature Sensor
------------------------------------------------------*/
void temperature(){
  temperature_raw = analogRead(TEMP);
  Serial.println(temperature_raw);
}
