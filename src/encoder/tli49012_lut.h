#pragma once
// Replace using tools/encoder/build_tli49012_lut.py after reference measurements.
// No MAQ473 calibration is valid for the TLI49012/magnet assembly.
// false이면 빈 표를 보정값으로 사용하지 않는다. 새 자석/장착 상태에서 참조 측정한 뒤 교체한다.
// 값의 단위는 rad이며 순서는 측정각 0..2π의 128개 균등 노드다.
constexpr bool TLI49012_LUT_VALID=false;
constexpr float TLI49012_LUT[128]={};
