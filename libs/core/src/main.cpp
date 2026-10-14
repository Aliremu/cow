#include "cow.h"
#include <spdlog/sinks/stdout_color_sinks.h>

auto main() -> int {
  auto a = cow<int>{10};
  auto logger = spdlog::stdout_color_mt("cow");
  logger->info("{}", *a._data.get());
}
