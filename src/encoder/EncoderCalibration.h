#pragma once
#include "TLI49012Sensor.h"
namespace EncoderCalibration {
// Motor remains disabled. User rotates the shaft by hand; no automatic motion.
// 호출자가 모터를 먼저 disable해야 한다. 센서 내부의 기본 SysCal 진행을 관찰할 뿐
// 모터/설정 레지스터/NVM을 조작하지 않는다. 취소·오류·시간초과는 false다.
bool observeSysCal(TLI49012Sensor& sensor,uint32_t timeoutMs=60000);
void printLUT(const TLI49012Sensor& sensor);
}
