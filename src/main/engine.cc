#include "main/engine.h"

#include "base/log/logger.h"
#include "main/utils/obj_string.h"

namespace traa {
namespace main {

engine::engine() { LOG_API_NO_ARGS(); }

engine::~engine() { LOG_API_NO_ARGS(); }

int engine::init(const traa_config *config) {
  LOG_API_ONE_ARG(traa::main::obj_string::to_string(config));

  return traa_error::TRAA_ERROR_NONE;
}

int engine::set_event_handler(const traa_event_handler *event_handler) {
  LOG_API_ONE_ARG(traa::main::obj_string::to_string(event_handler));

  return traa_error::TRAA_ERROR_NONE;
}

} // namespace main
} // namespace traa
