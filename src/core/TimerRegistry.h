#pragma once

#include <chrono>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "core/AST.h"

class TimerRegistry
{
public:
  enum class Kind {
    Monotonic,
    Cpu,
  };

  int create_timer(const std::string& name, Kind kind);

  void start_timer(const std::string& document_root, int id, const Location& loc);
  void clear_timer(const std::string& document_root, int id, const Location& loc);
  double stop_timer(const std::string& document_root, int id, const Location& loc);
  double elapsed_timer(const std::string& document_root, int id, const Location& loc);
  void delete_timer(const std::string& document_root, int id, const Location& loc);
  const std::string& timer_name(const std::string& document_root, int id, const Location& loc) const;

private:
  struct Timer {
    enum class State { Stopped, Running };

    int id = 0;
    std::string name;
    Kind kind = Kind::Monotonic;
    State state = State::Stopped;
    double elapsed_us = 0.0;
    std::chrono::steady_clock::time_point steady_start;
    std::clock_t cpu_start = 0;
  };

  std::vector<int> free_ids;
  std::vector<std::optional<Timer>> timers;
};
