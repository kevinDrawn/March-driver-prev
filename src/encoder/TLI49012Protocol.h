#pragma once
#include <stdint.h>
#include <stddef.h>

// Infineon TLx49012 UM Rev 1.00, sections 5.1, 7.1.1 and 8.2.
// 하드웨어 접근 없이 프레임/CRC만 계산하므로 호스트 테스트에서도 같은 코드를 사용한다.
namespace TLI49012 {
enum Register : uint8_t {
  // Predicted는 제어용 동적 보상각, Raw는 MCU LUT 이전 값과 다른 '칩 내부 단계'다.
  DeviceStatus=0x00, Diagnosis0=0x01, Diagnosis1=0x03,
  AnglePredicted=0x0C, Angle=0x0F, AngleRaw=0x10,
  FieldMagnitude=0x11, FieldX=0x12, FieldY=0x13,
  Angle3=0x14, FieldMagnitude3=0x15, FieldX3=0x16, FieldY3=0x17,
  SyscalOffsetX=0x2C, SyscalOffsetY=0x2D,
  SyscalEllipticity=0x2E, SyscalRotation=0x2F,
  Configuration10=0x48
};
enum Status : uint8_t {
  // 매 응답의 상위 byte 상태. CAL=0 자체는 통신 실패를 의미하지 않는다.
  Calibrated=0x01, Overtemperature=0x02, Reset=0x04, Supply=0x08,
  Magnet=0x10, Diagnosis=0x20, LastFrameOK=0x40, FrameCorrupted=0x80
};
inline uint8_t crc8(const uint8_t* data, size_t count) {
  // SAE-J1850: MSB-first, poly=0x1D, 초기값/최종 XOR=0xFF, 비트 반사 없음.
  uint8_t crc=0xFF;
  for (size_t i=0; i<count; ++i) {
    crc ^= data[i];
    for (unsigned b=0; b<8; ++b)
      crc = (crc & 0x80) ? uint8_t((crc << 1) ^ 0x1D) : uint8_t(crc << 1);
  }
  return crc ^ 0xFF;
}
inline uint32_t frame(uint32_t payload) {
  // [31:8]의 payload 24bit 뒤에 [7:0] CRC를 붙여 송신할 32bit 워드를 만든다.
  uint8_t bytes[3]={uint8_t(payload>>16),uint8_t(payload>>8),uint8_t(payload)};
  return (payload<<8)|crc8(bytes,3);
}
inline uint32_t readFrame(uint8_t address, uint8_t clearStatus=0) {
  // 최종 프레임 [31:25]=주소, [24]=읽기(0), [23:16]=0, [15:8]=상태 clear mask.
  return frame((uint32_t(address)<<17)|clearStatus);
}
inline uint32_t nextReadCommand(uint16_t address) {
  // 확장 주소 [23:10]와 CMD=00(read + address increment)를 0xFF 명령으로 설정한다.
  return frame(0xFF0000UL|(uint32_t(address)<<2));
}
inline uint32_t nextReadData(uint8_t clearStatus=0) {
  // 0xFD는 앞서 지정한 next-frame 주소의 데이터를 수신하는 프레임이다.
  return frame(0xFD0000UL|clearStatus);
}
inline bool validResponse(uint32_t tx, uint32_t rx) {
  // MISO CRC includes the MOSI address WITHOUT the R/W bit.
  // 센서가 접근 불가를 알리려고 응답 CRC를 반전한 경우도 여기서 실패로 검출된다.
  uint8_t bytes[4] = {uint8_t(tx>>25), uint8_t(rx>>24), uint8_t(rx>>16), uint8_t(rx>>8)};
  return crc8(bytes,4) == uint8_t(rx);
}
inline bool validAngleStatus(uint8_t status) {
  // 마지막 MOSI 정상 여부와 오류/경고 이력을 확인한다. CAL은 별도로 관찰한다.
  return (status & LastFrameOK) && !(status &
    (FrameCorrupted | Diagnosis | Magnet | Supply | Reset | Overtemperature));
}
}
