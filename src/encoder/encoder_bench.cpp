#ifdef ENCODER_TEST_MODE
// 엔코더만 검증하는 별도 진입점. PlatformIO source filter가 모터 앱을 제외한다.
#include "../config.h"
#include "TLI49012Sensor.h"
#include "EncoderCalibration.h"
#include "tli49012_lut.h"
SPIClass spiEncoder(PIN_ENCODER_MOSI,PIN_ENCODER_MISO,PIN_ENCODER_SCK,-1);
// 실제 센서는 하나이며 SPI 버스 객체는 이 시험 펌웨어가 소유한다.
TLI49012Sensor sensor(PIN_ENCODER_CS,&spiEncoder,ENCODER_SPI_CLOCK);
EncoderCalibration::LutBuilder builder;
static bool stream=false;
void setup() {
  // 출력 래치를 HIGH로 선설정한 뒤 출력 모드로 바꿔 DRVOFF의 LOW 글리치를 줄인다.
  // setup 전 reset 구간의 핀 레벨까지 이 코드가 보장하는 것은 아니다.
  digitalWrite(PIN_DRVOFF,HIGH); pinMode(PIN_DRVOFF,OUTPUT);
  Serial.begin(SERIAL_BAUD); Serial.setTimeout(10);
  delay(1000);
  // 검토된 저장 표가 있는 경우에만 적용한다. 실패하면 센서 내부 보상만 사용한다.
  if(TLI49012_LUT_VALID && !sensor.loadLUT(TLI49012_LUT,128))
    Serial.println("[ENC] Saved LUT invalid; host correction disabled.");
  sensor.init();
  Serial.println("TLI49012 bench: DRVOFF stays HIGH; no PWM/motor initialization.");
  Serial.println("d=diagnostics r=registers s=stream c=manual SysCal i=reinit");
  Serial.println("L measured_deg,reference_deg | Lf=build/apply | Lc=clear | Lp=print");
  sensor.runDiagnostics();
}
void loop() {
  if(Serial.available()) {
    String command=Serial.readStringUntil('\n'); command.trim();
    if(command=="d") sensor.runDiagnostics();
    else if(command=="r") sensor.dumpAllRegisters();
    else if(command=="s") stream=!stream;
    else if(command=="i") sensor.init();
    else if(command=="c") EncoderCalibration::observeSysCal(sensor);
    else if(command=="Lc") { builder.reset(); sensor.use_lut=false; Serial.println("LUT collection reset"); }
    else if(command=="Lp") EncoderCalibration::printLUT(sensor);
    else if(command=="Lf") {
      // 임시 배열에서 전체 품질 검사를 끝낸 다음에만 센서의 활성 보정표를 교체한다.
      float errors[EncoderCalibration::LutSize];
      bool ok=builder.finish(errors) && sensor.loadLUT(errors,EncoderCalibration::LutSize);
      Serial.print("LUT valid/applied="); Serial.print(ok);
      Serial.print(" coverage="); Serial.println(builder.coverage());
    } else if(command.startsWith("L ")) {
      // 입력은 측정각,독립 참조각 순서의 degree. 계산 모듈에는 rad로 변환해 전달한다.
      float measured,reference; char tail;
      // Reject malformed and extra input instead of String::toFloat returning zero.
      if(sscanf(command.c_str()+2,"%f,%f %c",&measured,&reference,&tail)==2 &&
         builder.add(measured*EncoderCalibration::TwoPi/360,reference*EncoderCalibration::TwoPi/360))
        Serial.println("pair accepted");
      else Serial.println("invalid pair");
    }
  }
  static uint32_t lastPrint=0;
  // 10ms 간격의 관찰용 출력이다. 이 주기로 모터 제어 루프를 구동하지 않는다.
  if(stream && uint32_t(millis()-lastPrint)>=10) {
    lastPrint=millis();
    TLI49012Sensor::Sample sample;
    bool valid=sensor.readSample(sample);
    // CSV: 시각(ms),유효성,16bit 내부각,내부각(rad),MCU 보정각(rad),상태(hex).
    // valid=0인 행은 오류 진단용이며 그 행의 각도를 측정값으로 사용하면 안 된다.
    Serial.print(lastPrint); Serial.print(','); Serial.print(valid); Serial.print(',');
    Serial.print(sample.angle16); Serial.print(',');
    Serial.print(sample.radians,6); Serial.print(',');
    Serial.print(valid && sensor.use_lut?sensor.applyLUT(sample.radians):sample.radians,6);
    Serial.print(','); Serial.println(sample.status,HEX);
  }
}
#endif
