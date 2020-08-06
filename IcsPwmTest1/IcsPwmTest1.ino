#include <ICS.h>

IcsController ICS(Serial1);
IcsServo servos[8];

void setup() {
  
  Serial.begin(115200);
  
  ICS.begin();
  for(int id=0; id<8; id++){
      servos[id].attach(ICS, id);
  }
  for(int id=0; id<8; id++){
      servos[id].setPosition(7500);
  }
  
  delay(1000);
}

void loop() {
  
  int position;
  
#if 1
  // (1) position parameter 3500 - 11500 (ICS compatible mode)
  for(int id=0; id<8; id++){
    for(int position=7500; position>=3500; position-=100){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=3500; position<=11500; position+=100){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=11500; position>=7500; position-=100){
      servos[id].setPosition(position);
      delay(20);
    }
  }
#else
  // (2) position parameter 500 - 2500 (microsecond mode)
  for(int id=0; id<8; id++){
    for(int position=1500; position>=600; position-=25){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=600; position<=2400; position+=25){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=2400; position>=1500; position-=25){
      servos[id].setPosition(position);
      delay(20);
    }
  }
#endif
}
