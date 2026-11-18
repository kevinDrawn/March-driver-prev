#include "../src/encoder/AngleCalibration.h"
#include "../src/encoder/TLI49012Protocol.h"
#include <assert.h>
#include <stdio.h>
#include <limits>
#include <initializer_list>
int main() {
  using namespace EncoderCalibration;
  const uint8_t standard[]={'1','2','3','4','5','6','7','8','9'};
  assert(TLI49012::crc8(standard,9)==0x4B); // CRC-8/SAE-J1850 check value.
  assert(TLI49012::readFrame(0x0C)==0x180000E5UL); // UM figure 19.
  assert(TLI49012::frame(0xF14711)==0xF1471135UL); // UM unlock example.
  assert(TLI49012::nextReadCommand(0x0C)==TLI49012::frame(0xFF0030));
  uint32_t tx=TLI49012::readFrame(0x0C);
  uint8_t bytes[]={0x0C,0x41,0x12,0x34};
  uint32_t rx=0x41123400UL|TLI49012::crc8(bytes,4);
  assert(TLI49012::validResponse(tx,rx));
  for(unsigned bit=0;bit<32;++bit) assert(!TLI49012::validResponse(tx,rx^(1UL<<bit)));
  assert(!TLI49012::validResponse(TLI49012::readFrame(0x0E),rx));
  assert(TLI49012::validAngleStatus(0x40)); // CAL may be pending at startup.
  assert(TLI49012::validAngleStatus(0x41));
  assert(!TLI49012::validAngleStatus(0));
  for(uint8_t bit: {uint8_t(2),uint8_t(4),uint8_t(8),uint8_t(16),uint8_t(32),uint8_t(128)})
    assert(!TLI49012::validAngleStatus(0x40|bit));
  float identity[LutSize]={};
  assert(validate(identity));
  assert(fabsf(difference(apply(-0.01f,identity),TwoPi-0.01f))<1e-5f);
  assert(fabsf(difference(apply(TwoPi+0.01f,identity),0.01f))<1e-5f);
  assert(fabsf(difference(0.01f,TwoPi-0.01f)-0.02f)<1e-5f);
  LutBuilder builder;
  assert(!builder.finish(identity)); // No empty/partial revolution calibration.
  assert(!builder.add(std::numeric_limits<float>::quiet_NaN(),0));
  // Two different offsets (including one crossing zero), periodic distortion,
  // forward and reverse reference measurements across several revolutions.
  for(float offset: {0.0f,0.4f}) {
    builder.reset();
    for(int direction: {1,-1}) for(unsigned i=0;i<4096;++i) {
      float reference=direction*float(i)*TwoPi/4096;
      float measured=wrap(reference+offset+0.04f*sinf(2*reference)+0.01f*cosf(3*reference));
      assert(builder.add(measured,reference));
    }
    float errors[LutSize]; assert(builder.finish(errors));
    float maxError=0;
    for(unsigned i=0;i<8192;++i) {
      float ref=float(i)*TwoPi/8192;
      float measured=wrap(ref+offset+0.04f*sinf(2*ref)+0.01f*cosf(3*ref));
      maxError=fmaxf(maxError,fabsf(difference(apply(measured,errors),ref)));
    }
    assert(maxError<0.001f);
    printf("Synthetic LUT maximum residual: %.6f rad\n",maxError);
  }
  float bad[LutSize]={}; bad[4]=1;
  assert(!validate(bad)); // Non-monotonic correction must not reach FOC.
  builder.reset();
  for(unsigned i=0;i<80;++i) builder.add(float(i)*TwoPi/LutSize,0);
  assert(!builder.finish(identity));
  puts("Encoder CRC, status, angle wrapping and calibration tests passed.");
}
