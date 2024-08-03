#include "traa/traa.h"

#include "base/log/logger.h"
#include "base/thread/task_queue.h"
#include "main/engine.h"
#include "utils/obj_string.h"

#include <memory>
#include <mutex>

namespace {
// The main queue id.
static const traa::base::task_queue::task_queue_id g_main_queue_id = 0;

// The main queue name.
static const char *g_main_queue_name = "traa_main";

// TODO @sylar: how to remove this mutex?
// The main queue mutex.
static std::mutex g_main_queue_mutex;

// The engine instance.
// The engine instance is created when traa_init is called and deleted when traa_release is called.
// The engine instance is a thread local variable to avoid the need for locking when accessing it,
// so do not use it outside of the main queue.
thread_local traa::main::engine *g_engine_instance = nullptr;
} // namespace

int traa_init(const traa_config *config) {
  if (config == nullptr) {
    LOG_ERROR("traa_config is null");
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (config->log_config.log_file != nullptr) {
    traa::base::logger::set_log_file(config->log_config.log_file, config->log_config.max_size,
                                     config->log_config.max_files);
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(config->log_config.level));
  }

  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(config));

  std::lock_guard<std::mutex> lock(g_main_queue_mutex);

  // no need to lock here coz we have rw lock in task_queue_manager
  auto main_queue = traa::base::task_queue_manager::get_task_queue(g_main_queue_id);
  if (!main_queue) {
    main_queue = traa::base::task_queue_manager::create_queue(g_main_queue_id, g_main_queue_name);
  }

  if (!main_queue) {
    LOG_FATAL("failed to create main queue");
    return traa_error::TRAA_ERROR_UNKNOWN;
  }

  auto wait_res = traa::base::task_queue_manager::post_task(g_main_queue_id, [&config]() {
    if (g_engine_instance == nullptr) {
      g_engine_instance = new traa::main::engine();
    }

    if (g_engine_instance == nullptr) {
      LOG_FATAL("failed to create engine instance");
      return static_cast<int>(traa_error::TRAA_ERROR_UNKNOWN);
    }

    int ret = g_engine_instance->init(config);
    if (ret != traa_error::TRAA_ERROR_NONE && ret != traa_error::TRAA_ERROR_ALREADY_INITIALIZED) {
      // to make sure that the engine instance is deleted if the engine initialization failed.
      delete g_engine_instance;
      g_engine_instance = nullptr;
    }

    return ret;
  });

  int ret = traa_error::TRAA_ERROR_UNKNOWN;
  // TODO @sylar: implement wait_res::wait_or_value() to avoid the need for adjust whether the
  // result is valid or not.
  if (wait_res.valid()) {
    ret = wait_res.get();
    if (ret != traa_error::TRAA_ERROR_NONE && ret != traa_error::TRAA_ERROR_ALREADY_INITIALIZED) {
      // to make sure that the main queue is released if the engine initialization failed.
      // so that we do not need to adjust the engine is exist or not in other places.
      traa::base::task_queue_manager::release_queue(g_main_queue_id);
    }
  }

  return ret;
}

void traa_release() {
  LOG_API_NO_ARGS();

  std::lock_guard<std::mutex> lock(g_main_queue_mutex);

  auto wait_res = traa::base::task_queue_manager::post_task(g_main_queue_id, []() {
    delete g_engine_instance;
    g_engine_instance = nullptr;
  });

  if (wait_res.valid()) {
    wait_res.wait();
  }

  traa::base::task_queue_manager::shutdown();
}

int traa_set_event_handler(const traa_event_handler *event_handler) {
  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(event_handler));

  if (event_handler == nullptr) {
    return traa_error::TRAA_ERROR_INVALID_ARGUMENT;
  }

  std::lock_guard<std::mutex> lock(g_main_queue_mutex);

  auto wait_res = traa::base::task_queue_manager::post_task(g_main_queue_id, [&event_handler]() {
    return g_engine_instance->set_event_handler(event_handler);
  });

  if (!wait_res.valid()) {
    return traa_error::TRAA_ERROR_NOT_INITIALIZED;
  }

  return wait_res.get();
}

void traa_set_log_level(traa_log_level level) {
  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(level));

  traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(level));
}

int traa_set_log(const traa_log_config *log_config) {
  LOG_API_ONE_ARG(traa::utils::obj_string::to_string(log_config));

  if (log_config == nullptr) {
    return TRAA_ERROR_INVALID_ARGUMENT;
  }

  if (log_config->log_file != nullptr) {
    // call set_level before set_log_file to ensure that no log messages written to the file or
    // stdout in case that user sets the log level to a higher level.
    traa::base::logger::set_level(static_cast<spdlog::level::level_enum>(log_config->level));
    traa::base::logger::set_log_file(log_config->log_file, log_config->max_size,
                                     log_config->max_files);
  }

  return TRAA_ERROR_NONE;
}