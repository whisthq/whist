#include "api/units/time_delta.h"
#include "api/units/data_rate.h"
#include "common_fixes.h"
#include <sstream>

bool g_in_slow_increase=0;

namespace webrtc {

std::string ToString(TimeDelta value) {
  std::stringstream sb;
  if (value.IsPlusInfinity()) {
    sb << "+inf ms";
  } else if (value.IsMinusInfinity()) {
    sb << "-inf ms";
  } else {
    if (value.us() == 0 || (value.us() % 1000) != 0)
      sb << value.us() << " us";
    else if (value.ms() % 1000 != 0)
      sb << value.ms() << " ms";
    else
      sb << value.seconds() << " s";
  }
  return sb.str();
}
std::string ToString(DataRate value) {
  std::stringstream sb;
  if (value.IsPlusInfinity()) {
    sb << "+inf bps";
  } else if (value.IsMinusInfinity()) {
    sb << "-inf bps";
  } else {
    if (value.bps() == 0 || value.bps() % 1000 != 0) {
      sb << value.bps() << " bps";
    } else {
      sb << value.kbps() << " kbps";
    }
  }
  return sb.str();
}
std::string ToString(DataSize value) {
  std::stringstream sb;
  if (value.IsPlusInfinity()) {
    sb << "+inf bytes";
  } else if (value.IsMinusInfinity()) {
    sb << "-inf bytes";
  } else {
    sb << value.bytes() << " bytes";
  }
  return sb.str();
}

}  // namespace webrtc

namespace my_logger
{
MyCout my_cout;
};
