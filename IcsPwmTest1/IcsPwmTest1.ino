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
      servos[id].setPosition(90);
  }
  
  delay(1000);
}

void loop() {
  
  int position;
  
  for(int id=0; id<8; id++){
    for(int position=90; position>=0; position-=2){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=0; position<=180; position+=2){
      servos[id].setPosition(position);
      delay(20);
    }
    for(int position=180; position>=90; position-=2){
      servos[id].setPosition(position);
      delay(20);
    }
  }
}
