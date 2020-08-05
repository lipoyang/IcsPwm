#include <ICS.h>

IcsController ICS(Serial1);
IcsServo servos[8];
int servoSel = 0;
int swOld = HIGH;

void setup() {
  
  Serial.begin(115200);
  
  ICS.begin();
  for(int id=0; id<8; id++){
      servos[id].attach(ICS, id);
  }
  for(int id=0; id<8; id++){
      servos[id].setPosition(7500);
  }
  
  pinMode(3, INPUT_PULLUP);
  analogReference(RAW12BIT);
  
  delay(1000);
}

void loop() {
  
  int sw = digitalRead(3);
  if(swOld == HIGH && sw == LOW){
    servoSel++;
    if(servoSel >= 8) servoSel = 0;
  }
  swOld = sw;
  
  int volume = analogRead(A0);
  int position = map(volume, 0, 4095, 3500, 11500);
  
  servos[servoSel].setPosition(position);
  delay(20);
}
