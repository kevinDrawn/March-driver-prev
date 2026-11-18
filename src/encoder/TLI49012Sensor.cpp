#include "TLI49012Sensor.h"
#include <string.h>

TLI49012Sensor::TLI49012Sensor(int cs,SPIClass* spi,uint32_t clock)
 // TLI49012는 기존 MAQ473(mode 3)과 다른 mode 1을 사용하며 최대 10MHz로 제한한다.
 :_cs(cs),_spi(spi),_settings(clock>10000000 ? 10000000 : clock,MSBFIRST,SPI_MODE1) {}

uint32_t TLI49012Sensor::transfer(uint32_t tx) {
  _spi->beginTransaction(_settings);
  digitalWrite(_cs,LOW);
  delayMicroseconds(1); // >=100 ns CS setup
  uint32_t rx=0;
  // CS를 유지한 채 상위 바이트부터 4byte를 전송해야 하나의 32bit 프레임이 된다.
  for(int shift=24;shift>=0;shift-=8) rx=(rx<<8)|_spi->transfer(uint8_t(tx>>shift));
  delayMicroseconds(1); // >=100 ns CS hold
  digitalWrite(_cs,HIGH);
  _spi->endTransaction();
  delayMicroseconds(1); // >=500 ns CS high
  return rx;
}
bool TLI49012Sensor::readRegister(uint8_t address,uint16_t& value,uint8_t clearStatus) {
  if(address>0x79) return false;
  bool inFrame=address>=0x0C && address<=0x17;
  // in-frame은 같은 프레임에서 값이 오고, next-frame은 주소 명령 후 데이터 프레임이 필요하다.
  uint32_t tx=inFrame ? TLI49012::readFrame(address,clearStatus) : TLI49012::nextReadData(clearStatus);
  for(unsigned attempt=0;attempt<2;++attempt) {
    // next-frame 재시도에서는 내부 주소 진행 상태에 의존하지 않도록 명령부터 다시 보낸다.
    if(!inFrame) {
      uint32_t command=TLI49012::nextReadCommand(address);
      uint32_t response=transfer(command);
      if(!TLI49012::validResponse(command,response)) { ++_crcErrors; continue; }
    }
    uint32_t rx=transfer(tx);
    if(TLI49012::validResponse(tx,rx)) {
      // 응답 [31:24]=상태, [23:8]=16bit 값, [7:0]=CRC. 상태 유효성은 호출자가 판단한다.
      _status=uint8_t(rx>>24); value=uint16_t(rx>>8); return true;
    }
    ++_crcErrors;
  }
  ++_errors; return false;
}
void TLI49012Sensor::init() {
  _initialized=false; _errors=_crcErrors=_failures=0;
  digitalWrite(_cs,HIGH); pinMode(_cs,OUTPUT);
  _spi->begin();
  // 전원/인터페이스 시작 후 첫 측정값이 준비될 시간을 확보한다.
  delay(10);
  // Prime LFOK, then explicitly acknowledge startup reset/frame history.
  uint16_t discarded;
  readRegister(TLI49012::AnglePredicted,discarded);
  if(!readRegister(TLI49012::AnglePredicted,discarded,TLI49012::Reset|TLI49012::FrameCorrupted)) return;
  Sample sample;
  if(!readSample(sample)) return;
  _initialized=true;
  // 베이스 클래스가 각도/속도 계산용 이전 샘플을 채운다. 모터를 구동하는 함수는 아니다.
  Sensor::init();
}
bool TLI49012Sensor::readSample(Sample& sample) {
  // 호출자의 이전 정상 데이터가 실패 샘플로 재사용되지 않도록 먼저 비운다.
  sample=Sample{};
  if(!readRegister(TLI49012::AnglePredicted,sample.angle16)) { ++_failures; return false; }
  sample.status=_status;
  // CRC 통과와 자기장/전원/진단 상태 통과는 별도 조건이다.
  if(!TLI49012::validAngleStatus(_status)) { ++_errors; ++_failures; return false; }
  sample.radians=float(sample.angle16)*(EncoderCalibration::TwoPi/65536.0f);
  sample.valid=true;
  _lastAngle=sample.angle16; _lastGood=micros(); _failures=0;
  return true;
}
bool TLI49012Sensor::healthy() const {
  // unsigned 차이는 micros() 카운터가 한 번 순환해도 짧은 경과시간을 올바르게 계산한다.
  return _initialized && _failures<3 && uint32_t(micros()-_lastGood)<100000;
}
float TLI49012Sensor::getSensorAngle() {
  Sample sample;
  if(!readSample(sample)) return -1;
  // 칩 내부 보상과 MCU 보정을 분리한다. 기본값은 내부 보상만 사용한다.
  return use_lut ? applyLUT(sample.radians) : sample.radians;
}
uint16_t TLI49012Sensor::readRawAngle() {
  Sample sample;
  return readSample(sample) ? sample.angle16 : _lastAngle;
}
float TLI49012Sensor::applyLUT(float angle) const { return EncoderCalibration::apply(angle,lut_error); }
bool TLI49012Sensor::loadLUT(const float* errors,size_t count) {
  if(!errors || count!=LUT_SIZE || !EncoderCalibration::validate(errors)) return false;
  memcpy(lut_error,errors,sizeof(lut_error)); use_lut=true; return true;
}
bool TLI49012Sensor::readField(int16_t& x,int16_t& y,uint16_t& magnitude,bool compensated) {
  // 모든 조회가 성공했을 때만 출력 인자를 갱신해 일부만 새 값인 결과를 방지한다.
  uint16_t a,b,m;
  if(!readRegister(compensated?TLI49012::FieldX:TLI49012::FieldX3,a) ||
     !TLI49012::validAngleStatus(_status) ||
     !readRegister(compensated?TLI49012::FieldY:TLI49012::FieldY3,b) ||
     !TLI49012::validAngleStatus(_status) ||
     !readRegister(compensated?TLI49012::FieldMagnitude:TLI49012::FieldMagnitude3,m) ||
     !TLI49012::validAngleStatus(_status)) return false;
  x=int16_t(a); y=int16_t(b); magnitude=m; return true;
}
bool TLI49012Sensor::readFieldAngle(float& radians,bool compensated) {
  int16_t x,y; uint16_t magnitude;
  if(!readField(x,y,magnitude,compensated) || (!x && !y)) return false;
  // 영벡터는 방향을 정의할 수 없다. atan2의 음수 결과는 [0, 2π)로 정규화한다.
  radians=EncoderCalibration::wrap(atan2f(float(y),float(x)));
  return true;
}
bool TLI49012Sensor::isMagnetInRange() {
  Sample sample; return readSample(sample) && !(sample.status&TLI49012::Magnet);
}
void TLI49012Sensor::dumpAllRegisters() {
  // 설정/NVM을 쓰지 않고 진단에 필요한 측정·상태 레지스터만 출력한다.
  const uint8_t addresses[]={0x00,0x01,0x03,0x0C,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17};
  Serial.println("[TLI49012] Read-only measurement/status registers:");
  for(uint8_t address:addresses) {
    uint16_t value;
    Serial.print("0x"); Serial.print(address,HEX); Serial.print(" = ");
    if(readRegister(address,value)) { Serial.print("0x"); Serial.println(value,HEX); }
    else Serial.println("CRC/communication error");
  }
}
void TLI49012Sensor::runDiagnostics() {
  Sample sample;
  bool ok=readSample(sample);
  Serial.print("[TLI49012] valid="); Serial.print(ok);
  Serial.print(" status=0x"); Serial.print(_status,HEX);
  Serial.print(" SysCal="); Serial.print(calibrated());
  Serial.print(" CRC errors="); Serial.println(_crcErrors);
  int16_t x,y; uint16_t magnitude;
  if(readField(x,y,magnitude)) {
    Serial.print("X3/Y3 counts: "); Serial.print(x); Serial.print('/'); Serial.println(y);
    Serial.print("B3 [mT]: "); Serial.println(magnitude*0.0064f,3);
    // 자기장 크기의 1count = 6.4µT = 0.0064mT. atan2는 내부 예측각과 비교하는 진단값이다.
    Serial.print("atan2(Y3,X3) diagnostic [rad]: ");
    Serial.println(EncoderCalibration::wrap(atan2f(float(y),float(x))),6);
  }
}
