#include "EncoderCalibration.h"
namespace EncoderCalibration {
bool observeSysCal(TLI49012Sensor& sensor,uint32_t timeoutMs) {
  Serial.println("[ENC-CAL] Rotate shaft by hand for at least 10 full turns; any key cancels.");
  Serial.println("[ENC-CAL] Default SysCal converges after 3 turns; CAL=1 reports active compensation.");
  while(Serial.available()) Serial.read();
  // 명령 c 뒤에 남은 줄바꿈을 비워 즉시 취소로 해석되지 않게 한다.
  uint32_t start=millis(),lastPrint=start;
  float travel=0,previous=0;
  bool havePrevious=false;
  while(uint32_t(millis()-start)<timeoutMs) {
    if(Serial.available()) { while(Serial.available()) Serial.read(); return false; }
    TLI49012Sensor::Sample sample;
    if(!sensor.readSample(sample)) { Serial.println("[ENC-CAL] Invalid sensor frame/status; aborted."); return false; }
    // 0/360° 경계는 최단 각도차로 처리한다. 빠른 회전으로 표본 간 π를 넘지 않아야 한다.
    if(havePrevious) travel+=fabsf(difference(sample.radians,previous));
    previous=sample.radians; havePrevious=true;
    if(uint32_t(millis()-lastPrint)>=500) {
      lastPrint=millis();
      Serial.print("[ENC-CAL] travel turns~"); Serial.print(travel/TwoPi,2);
      Serial.print(" CAL="); Serial.println(sensor.calibrated());
    }
    // Observed travel is approximate, not proof of complete angular coverage.
    // 절댓값 누적이므로 왕복/잡음도 이동량에 포함된다. 사용자가 실제 전 범위를 회전시켜야 한다.
    if(travel>=10*TwoPi && sensor.calibrated()) {
      Serial.println("[ENC-CAL] SysCal active; no NVM written. Host LUT unchanged.");
      sensor.runDiagnostics(); return true;
    }
    delay(2);
  }
  Serial.println("[ENC-CAL] Timeout. Full rotations and magnetic field must be checked.");
  return false;
}
void printLUT(const TLI49012Sensor& sensor) {
  // RAM 표를 헤더에 복사할 수 있게 출력한다. 출력만으로 Flash/NVM에 저장되지는 않는다.
  Serial.println("// Host TLI49012 LUT: error = measured - reference, radians");
  Serial.println("constexpr bool TLI49012_LUT_VALID=true;");
  Serial.println("constexpr float TLI49012_LUT[128] = {");
  for(unsigned i=0;i<LutSize;++i) {
    Serial.print(sensor.lut_error[i],8); Serial.print(i+1<LutSize?",":"");
    if(i%8==7) Serial.println();
  }
  Serial.println("};");
}
}
