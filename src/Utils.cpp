#include "Utils.h"
void handleNoChanceError(){
  Serial.println("❌❌❌ DAS HAT NICHT GEKLAPPT! Den selben Song nochmal! ❌❌❌");
  delay(200);
  ESP.restart();
}
bool isFiniteCoord(const String &s){
  if (s.isEmpty()) return false;
  bool dotSeen=false, digitSeen=false;
  int start=(s[0]=='-'||s[0]=='+')?1:0;
  for(int i=start;i<s.length();++i){
    char c=s[i];
    if(c=='.'){ if(dotSeen) return false; dotSeen=true; }
    else if(!isDigit(c)) return false;
    else digitSeen=true;
  }
  return digitSeen;
}