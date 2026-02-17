#include "core/TimerRegistry.h"

#include <chrono>
#include <ctime>
#include <string>

#include "utils/exceptions.h"
#include "utils/printutils.h"

static void timer_error(const std::string& document_root, const Location& loc, const std::string& msg)
{
  LOG(message_group::Error, loc, document_root, "%1$s", msg);
  throw EvaluationException(msg);
}

static double cpu_elapsed_us(std::clock_t start)
{
  return (static_cast<double>(std::clock() - start) * 1000000.0) / CLOCKS_PER_SEC;
}

static double steady_elapsed_us(const std::chrono::steady_clock::time_point& start)
{
  const auto now = std::chrono::steady_clock::now();
  const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
  return static_cast<double>(us);
}

int TimerRegistry::create_timer(const std::string& name, Kind kind)
{
  if (free_ids.empty() && timers.empty()) {
    free_ids.reserve(10);
    timers.reserve(10);
  }

  int id;
  if (!free_ids.empty()) {
    id = free_ids.back();
    free_ids.pop_back();
  } else {
    id = static_cast<int>(timers.size());
  }

  Timer timer;
  timer.id = id;
  timer.name = name;
  timer.kind = kind;
  timer.state = Timer::State::Stopped;
  timer.elapsed_us = 0.0;

  if (id == static_cast<int>(timers.size())) {
    timers.emplace_back(std::move(timer));
  } else {
    timers[id] = std::move(timer);
  }
  return id;
}

void TimerRegistry::start_timer(const std::string& document_root, int id, const Location& loc)
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_start(", id, ") unknown timer id"));
  }
  auto& timer = *timers[id];
  if (timer.state == Timer::State::Running) {
    timer_error(document_root, loc, STR("timer_start(", id, ") timer already running"));
  }
  if (timer.kind == Kind::Cpu) {
    timer.cpu_start = std::clock();
  } else {
    timer.steady_start = std::chrono::steady_clock::now();
  }
  timer.state = Timer::State::Running;
}

void TimerRegistry::clear_timer(const std::string& document_root, int id, const Location& loc)
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_clear(", id, ") unknown timer id"));
  }
  auto& timer = *timers[id];
  timer.elapsed_us = 0.0;
  timer.state = Timer::State::Stopped;
}

double TimerRegistry::stop_timer(const std::string& document_root, int id, const Location& loc)
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_stop(", id, ") unknown timer id"));
  }
  auto& timer = *timers[id];
  if (timer.state != Timer::State::Running) {
    timer_error(document_root, loc, STR("timer_stop(", id, ") timer not running"));
  }
  if (timer.kind == Kind::Cpu) {
    timer.elapsed_us += cpu_elapsed_us(timer.cpu_start);
  } else {
    timer.elapsed_us += steady_elapsed_us(timer.steady_start);
  }
  timer.state = Timer::State::Stopped;
  return timer.elapsed_us;
}

double TimerRegistry::elapsed_timer(const std::string& document_root, int id, const Location& loc)
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_elapsed(", id, ") unknown timer id"));
  }
  auto& timer = *timers[id];
  if (timer.state == Timer::State::Running) {
    if (timer.kind == Kind::Cpu) {
      return timer.elapsed_us + cpu_elapsed_us(timer.cpu_start);
    }
    return timer.elapsed_us + steady_elapsed_us(timer.steady_start);
  }
  return timer.elapsed_us;
}

void TimerRegistry::delete_timer(const std::string& document_root, int id, const Location& loc)
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_delete(", id, ") unknown timer id"));
  }
  timers[id].reset();
  free_ids.push_back(id);
}

const std::string& TimerRegistry::timer_name(const std::string& document_root, int id,
                                             const Location& loc) const
{
  if (id < 0 || id >= static_cast<int>(timers.size()) || !timers[id]) {
    timer_error(document_root, loc, STR("timer_name(", id, ") unknown timer id"));
  }
  return timers[id]->name;
}
