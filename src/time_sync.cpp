#include "time_sync.h"
#include <time.h>

namespace time_sync {

void beginGMT3() {
  const long gmtOffset_sec = 3 * 3600;
  const int daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
}

String timeHHMM() {
  time_t now = time(nullptr);
  if (now < 100000) return "--:--";
  struct tm* tm_info = localtime(&now);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
  return String(buf);
}

}
