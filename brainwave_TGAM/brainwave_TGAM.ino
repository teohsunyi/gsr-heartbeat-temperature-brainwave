#include <SD.h>
#include <SPI.h>

File myFile;

#define CS_PIN    53 

int read_allow=0;
int read_count=0;
int   generatedChecksum = 0;
byte  checksum = 0; 
byte  payloadLength = 0;
byte  payloadData[32] = {0};
byte signalquality = 0;
byte attention = 0;    
byte meditation = 0; 

void setup() {
  Serial.setRxBufferSize(5000);
  Serial.begin(57600);
}

void loop() {
  if(Serial.available()>=4990) read_allow = 1;
  if(read_allow){
    if(read_count>=4900){
      read_allow = 0;
      read_count = 0;
      Serial.end();
      Serial.begin(57600);
    }
      if(Serial.read() == 0xAA){
        read_count++;
        if(Serial.read() == 0xAA){
          read_count++;
          payloadLength = Serial.read();
          if(payloadLength == 0x20){       // correct data size id 32 byte
            read_count++;
            generatedChecksum = 0;
            Serial.print("read_count:");
            Serial.println(read_count);
            for(int i = 0; i < payloadLength; i++){  // read all the 32 bytes data
              payloadData[i] = Serial.read();
              Serial.print(payloadData[i],HEX);
              Serial.print(" ");
              read_count++;
              generatedChecksum += payloadData[i];   // sum up all the 32 byytes data 
            }         
            checksum = Serial.read();
            read_count++;
            generatedChecksum = (~generatedChecksum)&0xff;       
            if(checksum == generatedChecksum)//数据接收正确，继续处理 
            {    
              signalquality = 0;
              attention = 0;    
              meditation = 0;
              signalquality = payloadData[1];//信号值      
              attention = payloadData[29];   //注意力值
              meditation = payloadData[31];  //放松度值    
            } 
          }
          else read_count++;
        }
        else read_count++;
      }
      else read_count++;
      
  }

-------------------------------
SAVE THE COLLECTED INTO SD CARD
-------------------------------

  myFile = SD.open("data_log.txt", FILE_WRITE);
  if(myFile){
      myFile.print(read_count);
      myFile.print(",");
      myFile.print(payloadLength);
      myFile.print(",");
      myFile.print(signalquality);
      myFile.print(",");
      myFile.print(attention);
      myFile.print(",");
      myFile.print(meditation);
      myFile.println(",");
      myFile.close(); // close the file
  }
}
