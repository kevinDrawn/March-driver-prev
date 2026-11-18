#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <common/base_classes/Sensor.h>
#include "AngleCalibration.h"
#include "TLI49012Protocol.h"

// SPI 통신과 센서 상태를 관리하고, SimpleFOC에 기계각(rad)을 제공하는 어댑터.
// 인스턴스는 앱/시험 펌웨어 중 선택된 진입점에서 하나만 생성한다.
class TLI49012Sensor : public Sensor {
public:
  // 센서 내부 LUT와 별개인 MCU 측 보정표 크기.
  static constexpr int LUT_SIZE=EncoderCalibration::LutSize;
  // angle16/radians는 센서 내부 보상 후, MCU LUT 적용 전 값이다.
  // SPI/상태 검사에 실패하면 valid=false이므로 나머지 각도 필드를 사용하지 않는다.
  struct Sample {
    uint16_t angle16=0;
    uint8_t status=0;
    float radians=0;
    bool valid=false;
  };
  // SPIClass는 외부에서 생성하며 이 클래스는 포인터만 참조한다.
  // clock은 요청 SCK(Hz); 실제 주파수는 MCU의 분주 설정에 따른다.
  TLI49012Sensor(int cs,SPIClass* spi,uint32_t clock=4000000);
  // CS/SPI 준비, 부팅 상태 확인, SimpleFOC 각도 이력 초기화. 성공 여부는 healthy()로 확인.
  void init() override;
  float getSensorAngle() override; // -1 on invalid data: SimpleFOC skips the update.
  // 새 각도 한 개를 CRC와 장치 상태까지 검사하여 반환한다.
  bool readSample(Sample& sample);
  // 측정 레지스터는 in-frame, 그 외에는 next-frame으로 읽는다.
  // clearStatus의 1인 비트만 상태 해제를 요청한다. 기본 0은 오류 이력을 보존한다.
  bool readRegister(uint8_t address,uint16_t& value,uint8_t clearStatus=0);
  // 초기화 완료 + 연속 실패 3회 미만 + 마지막 정상 샘플이 100ms 이내인지 확인.
  bool healthy() const;
  // CAL은 내부 SysCal 보상이 활성이라는 뜻이며 절대 정확도 검증 결과는 아니다.
  bool calibrated() const { return (_status & TLI49012::Calibrated)!=0; }
  uint8_t deviceStatus() const { return _status; }
  uint32_t errorCount() const { return _errors; }
  uint32_t crcErrorCount() const { return _crcErrors; }
  // compensated=false: X3/Y3, true: SysCal 보상된 Xn/Yn. 값은 센서 counts 단위.
  // X/Y는 별도 프레임에서 읽으므로 고속 회전 시 같은 시각의 벡터가 아닐 수 있다.
  bool readField(int16_t& x,int16_t& y,uint16_t& magnitude,bool compensated=false);
  // 자기장 방향 진단용 atan2 결과. 제어용 예측 각도(getSensorAngle)와 구분한다.
  bool readFieldAngle(float& radians,bool compensated=true);
  // 개수·유한값·단조성을 검증한 표만 복사/활성화한다. 실패 시 기존 표는 유지한다.
  bool loadLUT(const float* errors,size_t count);
  float applyLUT(float angle) const;
  // 기존 앱 호환 API: MCU LUT 전 값이며 실패하면 마지막 정상값을 반환한다.
  // 실패 여부까지 필요한 새 코드는 readSample()을 사용한다.
  uint16_t readRawAngle(); // Uncorrected by HOST LUT; sensor SysCal is still applied.
  bool isMagnetInRange();
  void dumpAllRegisters();
  void runDiagnostics();
  // lut_error[i] = 측정각 - 참조각(rad), 측정각 기준으로 한 바퀴에 균등 배치.
  // 외부 표 적용 시 직접 배열을 수정하기보다 loadLUT()을 사용한다.
  bool use_lut=false;
  float lut_error[LUT_SIZE]={};
private:
  uint32_t transfer(uint32_t frame);
  int _cs;
  SPIClass* _spi;
  SPISettings _settings;
  bool _initialized=false;
  uint8_t _status=0;
  uint16_t _lastAngle=0;
  // 누적 오류/CRC 오류와 별도로 _failures는 연속 각도 실패, _lastGood은 micros() 시각.
  uint32_t _errors=0,_crcErrors=0,_failures=0,_lastGood=0;
};
