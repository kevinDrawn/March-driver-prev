#pragma once
#include "../config.h"
// 보드별 컴파일 설정에서 센서 타입을 한 번 선택한다. 앱은 구체적인 제품명 대신 이 타입을 쓴다.
#ifdef ENCODER_TLI49012
#include "TLI49012Sensor.h"
using EncoderSensor=TLI49012Sensor;
#else
#include "MAQ473Sensor.h"
using EncoderSensor=MAQ473Sensor;
#endif
// 실체는 모터 앱(main.cpp) 또는 단독 시험(encoder_bench.cpp) 중 하나에서만 정의한다.
extern EncoderSensor sensor;
