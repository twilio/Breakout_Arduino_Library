#include "BreakoutSDK/utils/log.h"
#include "BreakoutSDK/utils/time.h"

#include <stdarg.h>
#include <string>
#include <iostream>
#include <time.h>
#include <unistd.h>

static time_t initial_time = time(NULL);

void owl_log(log_level_t, char* format, ...) {
  char buf[2048];

  va_list ap;
  va_start(ap, format);

  int written = vsnprintf(buf, 2048, format, ap);

  std::cerr << "LOG: " << std::string(buf, written);
}

owl_time_t owl_time() {
  return (time(NULL) - initial_time) * 1000;
}

void owl_delay(uint32_t ms) {
  usleep(ms * 1000);
}
