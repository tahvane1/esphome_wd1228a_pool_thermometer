#include "esphome.h"

static const char *TAG = "POOLTEMPSENSOR";

#define RING_BUFFER_SIZE  256 

#define SYNC_LENGTH  4000 
#define SEP_LENGTH   500  
#define BIT1_LENGTH  2000 
#define BIT0_LENGTH  1000 


unsigned long timings[RING_BUFFER_SIZE];
unsigned int syncIndex1 = 0;  
unsigned int syncIndex2 = 0;  
bool received = false;
unsigned int datapin = 14;

class PoolTempSensor : public Component{
  

  private:
    String data_txt, prefix,postfix,channel,temp;
    static bool isSync(unsigned int idx) {
      unsigned long t0 = timings[(idx+RING_BUFFER_SIZE-1) % RING_BUFFER_SIZE];
      unsigned long t1 = timings[idx];  

      if (t0>(SEP_LENGTH-100) && t0<(SEP_LENGTH+100) &&
        t1>(SYNC_LENGTH-1000) && t1<(SYNC_LENGTH+1000) &&
        digitalRead(datapin) == HIGH) {
        return true;
      }
      return false;
    }

  /* Interrupt 1 handler */
    static void ICACHE_RAM_ATTR handler() {
      static unsigned long duration = 0;
      static unsigned long lastTime = 0;
      static unsigned int ringIndex = 0;
      static unsigned int syncCount = 0;

      if (received == true) {
        return;
      }
      long time = micros();
      duration = time - lastTime;
      lastTime = time;

      ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
      timings[ringIndex] = duration;

      if (isSync(ringIndex)) {
        syncCount ++;
        if (syncCount == 1) {
          syncIndex1 = (ringIndex+1) % RING_BUFFER_SIZE;
        }
        else if (syncCount == 2) {
          syncCount = 0;
          syncIndex2 = (ringIndex+1) % RING_BUFFER_SIZE;
          unsigned int changeCount = (syncIndex2 < syncIndex1) ? (syncIndex2+RING_BUFFER_SIZE - syncIndex1) : (syncIndex2 - syncIndex1);
          if (changeCount != 74) {
            received = false;
            syncIndex1 = 0;
            syncIndex2 = 0;
          }
          else {
            received = true;
          }
        }

      }
    }
    long binaryToDecimal(String value){
      int base = 2; 
      int length = value.length() +1;
      char valueAsArray[length];
      value.toCharArray(valueAsArray, length);
      
      return strtol(valueAsArray, NULL, base);
    }

  public:
    Sensor *temperature_sensor_channel1 = new Sensor();
    Sensor *temperature_sensor_channel2 = new Sensor();
    Sensor *temperature_sensor_channel3 = new Sensor();
    
    PoolTempSensor(int pin) : Component(){
      datapin = pin;
    }

    void setup() override {
      pinMode(datapin, INPUT); 
      attachInterrupt(digitalPinToInterrupt(datapin), handler, CHANGE);
    }
    void loop() override {
        if (received == true) {

        detachInterrupt(1);
        for(unsigned int i=syncIndex1; i!=syncIndex2; i=(i+2)%RING_BUFFER_SIZE) {
          unsigned long t0 = timings[i], t1 = timings[(i+1)%RING_BUFFER_SIZE];
          if (t0>(SEP_LENGTH-100) && t0<(SEP_LENGTH+100)) {
          if (t1>(BIT1_LENGTH-1000) && t1<(BIT1_LENGTH+1000)) {
            data_txt += "1"; // on insert un 1 dan la chaine
          } else if (t1>(BIT0_LENGTH-1000) && t1<(BIT0_LENGTH+1000)) {
            data_txt += "0"; // on insert un 0 dans la chaine

          } else {
          }
          } else {
          }
          
        }
    int input_size = data_txt.length(); 


    if(input_size == 36) { 

    prefix = data_txt.substring(0,8);
    channel = data_txt.substring(8,12);
    temp = data_txt.substring(12,24);
    postfix = data_txt.substring(24,36);

    long temp_as_long = binaryToDecimal(temp);
    float temp_as_float = temp_as_long / 10.;
    //Serial.println(temp_as_float);
    ESP_LOGD(TAG, "Temperature value %.1f", temp_as_float);
    if(channel == "1000") { 
      ESP_LOGD(TAG, "Channel 1");
      temperature_sensor_channel1->publish_state(temp_as_float);
    } 
    if(channel == "1001") { 
      temperature_sensor_channel2->publish_state(temp_as_float);
    } 
    if(channel == "1010") {
      temperature_sensor_channel3->publish_state(temp_as_float);
    }



    
    }

    //Serial.println(); ´

    data_txt = ""; 

        received = false;
        syncIndex1 = 0;
        syncIndex2 = 0;
        attachInterrupt(digitalPinToInterrupt(datapin), handler, CHANGE);
      }
    }
  
};