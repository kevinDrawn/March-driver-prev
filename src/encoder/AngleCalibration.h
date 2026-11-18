#pragma once
#include <math.h>
#include <stdint.h>
#include <stddef.h>

// Pure math, no MCU/SPI/motor ownership. LUT is indexed by measured angle.
// 각도/오차는 모두 rad 단위다. 칩 내부 SysCal/LUT 설정이나 모터 동작은 여기서 변경하지 않는다.
namespace EncoderCalibration {
constexpr float TwoPi = 6.2831853071795864769f;
constexpr unsigned LutSize = 128;
inline float wrap(float angle) {
  // 여러 회전 및 음수 입력을 [0, 2π) 범위로 정규화한다.
  angle=fmodf(angle,TwoPi);
  return angle<0 ? angle+TwoPi : angle;
}
inline float difference(float a,float b) {
  // a-b의 최단 원형 차이 [-π, π). 359°와 1° 차이를 358°로 취급하지 않는다.
  float d=wrap(a-b);
  return d>=TwoPi/2 ? d-TwoPi : d;
}
inline float apply(float angle,const float* errors) {
  // LUT의 인덱스는 참조각이 아닌 측정각이다. 마지막 점 다음에는 첫 점을 연결한다.
  float position=wrap(angle)*LutSize/TwoPi;
  unsigned i=unsigned(position)%LutSize;
  float fraction=position-floorf(position);
  float correction=errors[i]+fraction*difference(errors[(i+1)%LutSize],errors[i]);
  // 보정표는 '측정-참조' 오차이므로 측정각에서 빼야 참조각에 가까워진다.
  return wrap(angle-correction);
}
inline bool validate(const float* errors) {
  const float step=TwoPi/LutSize;
  for (unsigned i=0;i<LutSize;++i) {
    if (!isfinite(errors[i]) || fabsf(errors[i])>=TwoPi/4) return false;
    // 입력 한 구간(step)에 대한 출력 증가량이 양수이고 5% 이상 남도록 제한한다.
    // 표 때문에 각도 진행 방향이 뒤집히거나 거의 평평해지는 경우를 거부한다.
    // Corrected mapping must remain monotonic across every interval.
    if (step-difference(errors[(i+1)%LutSize],errors[i])<=step*0.05f) return false;
  }
  return true;
}
// 독립 참조 엔코더/지그의 표본을 수집한다. 모터 명령각을 실제 참조각으로 가정하지 않는다.
class LutBuilder {
public:
  void reset() { for(unsigned i=0;i<LutSize;++i) { s[i]=c[i]=0; n[i]=0; } }
  LutBuilder() { reset(); }
  bool add(float measured,float reference) {
    if (!isfinite(measured) || !isfinite(reference)) return false;
    // 각 측정값을 가장 가까운 LUT 노드로 모은다. +0.5는 내림 대신 반올림하기 위한 값이다.
    unsigned slot=unsigned(wrap(measured)*LutSize/TwoPi+0.5f)%LutSize;
    if (n[slot]==UINT16_MAX) return false;
    float e=difference(measured,reference);
    // 경계 부근 오차도 평균할 수 있도록 오차 자체 대신 sin/cos를 누적한다.
    s[slot]+=sinf(e); c[slot]+=cosf(e); ++n[slot]; return true;
  }
  unsigned coverage() const {
    unsigned count=0; for(unsigned i=0;i<LutSize;++i) if(n[i]) ++count; return count;
  }
  bool finish(float* errors) const {
    // 최소 75%(96/128)의 노드에 실제 표본이 있어야 한 바퀴 보정표를 생성한다.
    if (coverage()<LutSize*3/4) return false;
    for(unsigned i=0;i<LutSize;++i) {
      if(n[i]) {
        // 원형 평균의 집중도가 낮으면 같은 위치에서 참조값이 일관되지 않은 것으로 판단한다.
        if(hypotf(s[i],c[i])/n[i]<0.98f) return false;
        errors[i]=atan2f(s[i],c[i]);
      } else {
        // 표본 없는 짧은 구간만 좌우의 실제 표본 사이에서 보간한다.
        unsigned left=1,right=1;
        while(left<LutSize && !n[(i+LutSize-left)%LutSize]) ++left;
        while(right<LutSize && !n[(i+right)%LutSize]) ++right;
        if(left+right-1>4) return false;
        float a=atan2f(s[(i+LutSize-left)%LutSize],c[(i+LutSize-left)%LutSize]);
        float b=atan2f(s[(i+right)%LutSize],c[(i+right)%LutSize]);
        errors[i]=a+difference(b,a)*float(left)/float(left+right);
      }
    }
    return validate(errors);
  }
private:
  // s/c는 오차의 sin/cos 합, n은 노드별 표본 수다. finish() 전까지 보정표를 활성화하지 않는다.
  float s[LutSize],c[LutSize];
  uint16_t n[LutSize];
};
}
