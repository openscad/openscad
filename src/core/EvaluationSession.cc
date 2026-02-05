/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "core/EvaluationSession.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <ctime>

#include "core/AST.h"
#include "core/ContextFrame.h"
#include "core/Value.h"
#include "core/callables.h"
#include "core/function.h"
#include "core/module.h"
#include "core/Value.h"
#include "utils/exceptions.h"
#include "utils/printutils.h"

struct EvaluationSession::TimerRegistry {
  struct Timer {
    enum class State { Stopped, Running };

    int id = 0;
    std::string name;
    TimerType type = TimerType::Monotonic;
    State state = State::Stopped;
    double elapsed_ms = 0.0;
    std::chrono::steady_clock::time_point steady_start;
    std::clock_t cpu_start = 0;
  };

  std::vector<int> free_ids;
  std::vector<std::optional<Timer>> timers;
};

static void timer_error(const EvaluationSession& session, const Location& loc, const std::string& msg)
{
  LOG(message_group::Error, loc, session.documentRoot(), "%1$s", msg);
  throw EvaluationException(msg);
}

static double cpu_elapsed_ms(std::clock_t start)
{
  return (static_cast<double>(std::clock() - start) * 1000.0) / CLOCKS_PER_SEC;
}

static double steady_elapsed_ms(const std::chrono::steady_clock::time_point& start)
{
  const auto now = std::chrono::steady_clock::now();
  const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
  return static_cast<double>(us) / 1000.0;
}

size_t EvaluationSession::push_frame(ContextFrame *frame)
{
  size_t index = stack.size();
  stack.push_back(frame);
  return index;
}

EvaluationSession::EvaluationSession(std::string documentRoot) : document_root(std::move(documentRoot)) {}

EvaluationSession::~EvaluationSession() = default;

void EvaluationSession::replace_frame(size_t index, ContextFrame *frame)
{
  assert(index < stack.size());
  stack[index] = frame;
}

void EvaluationSession::pop_frame(size_t index)
{
  stack.pop_back();
  assert(stack.size() == index);
}

boost::optional<const Value&> EvaluationSession::try_lookup_special_variable(
  const std::string& name) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<const Value&> result = (*it)->lookup_local_variable(name);
    if (result) {
      return result;
    }
  }
  return boost::none;
}

const Value& EvaluationSession::lookup_special_variable(const std::string& name,
                                                        const Location& loc) const
{
  boost::optional<const Value&> result = try_lookup_special_variable(name);
  if (!result) {
    LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown variable %1$s", quoteVar(name));
    return Value::undefined;
  }
  return *result;
}

boost::optional<CallableFunction> EvaluationSession::lookup_special_function(const std::string& name,
                                                                             const Location& loc) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<CallableFunction> result = (*it)->lookup_local_function(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown function '%1$s'", name);
  return boost::none;
}

boost::optional<InstantiableModule> EvaluationSession::lookup_special_module(const std::string& name,
                                                                             const Location& loc) const
{
  for (auto it = stack.crbegin(); it != stack.crend(); ++it) {
    boost::optional<InstantiableModule> result = (*it)->lookup_local_module(name, loc);
    if (result) {
      return result;
    }
  }
  LOG(message_group::Warning, loc, documentRoot(), "Ignoring unknown module '%1$s'", name);
  return boost::none;
}

int EvaluationSession::timer_new(const std::string& name, TimerType type)
{
  if (!timer_registry) {
    timer_registry = std::make_unique<TimerRegistry>();
    timer_registry->free_ids.reserve(10);
    timer_registry->timers.reserve(10);
  }

  int id;
  if (!timer_registry->free_ids.empty()) {
    id = timer_registry->free_ids.back();
    timer_registry->free_ids.pop_back();
  } else {
    id = static_cast<int>(timer_registry->timers.size());
  }

  TimerRegistry::Timer timer;
  timer.id = id;
  timer.name = name;
  timer.type = type;
  timer.state = TimerRegistry::Timer::State::Stopped;
  timer.elapsed_ms = 0.0;

  if (id == static_cast<int>(timer_registry->timers.size())) {
    timer_registry->timers.emplace_back(std::move(timer));
  } else {
    timer_registry->timers[id] = std::move(timer);
  }
  return id;
}

void EvaluationSession::timer_start(int id, const Location& loc)
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_start(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_start(", id, ") unknown timer id"));
  }
  auto& timer = *timer_registry->timers[id];
  if (timer.state == TimerRegistry::Timer::State::Running) {
    timer_error(*this, loc, STR("timer_start(", id, ") timer already running"));
  }
  timer.elapsed_ms = 0.0;
  if (timer.type == TimerType::Cpu) {
    timer.cpu_start = std::clock();
  } else {
    timer.steady_start = std::chrono::steady_clock::now();
  }
  timer.state = TimerRegistry::Timer::State::Running;
}

void EvaluationSession::timer_clear(int id, const Location& loc)
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_clear(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_clear(", id, ") unknown timer id"));
  }
  auto& timer = *timer_registry->timers[id];
  timer.elapsed_ms = 0.0;
  timer.state = TimerRegistry::Timer::State::Stopped;
}

double EvaluationSession::timer_stop(int id, const Location& loc)
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_stop(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_stop(", id, ") unknown timer id"));
  }
  auto& timer = *timer_registry->timers[id];
  if (timer.state != TimerRegistry::Timer::State::Running) {
    timer_error(*this, loc, STR("timer_stop(", id, ") timer not running"));
  }
  if (timer.type == TimerType::Cpu) {
    timer.elapsed_ms = cpu_elapsed_ms(timer.cpu_start);
  } else {
    timer.elapsed_ms = steady_elapsed_ms(timer.steady_start);
  }
  timer.state = TimerRegistry::Timer::State::Stopped;
  return timer.elapsed_ms;
}

double EvaluationSession::timer_elapsed(int id, const Location& loc)
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_elapsed(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_elapsed(", id, ") unknown timer id"));
  }
  auto& timer = *timer_registry->timers[id];
  if (timer.state == TimerRegistry::Timer::State::Running) {
    if (timer.type == TimerType::Cpu) {
      return cpu_elapsed_ms(timer.cpu_start);
    }
    return steady_elapsed_ms(timer.steady_start);
  }
  return timer.elapsed_ms;
}

void EvaluationSession::timer_delete(int id, const Location& loc)
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_delete(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_delete(", id, ") unknown timer id"));
  }
  timer_registry->timers[id].reset();
  timer_registry->free_ids.push_back(id);
}

const std::string& EvaluationSession::timer_name(int id, const Location& loc) const
{
  if (!timer_registry) {
    timer_error(*this, loc, STR("timer_name(", id, ") unknown timer id"));
  }
  if (id < 0 || id >= static_cast<int>(timer_registry->timers.size()) ||
      !timer_registry->timers[id]) {
    timer_error(*this, loc, STR("timer_name(", id, ") unknown timer id"));
  }
  return timer_registry->timers[id]->name;
}
