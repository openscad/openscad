#include "platform/Subprocess.h"

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>

#include <cerrno>
#include <cstring>
#ifdef __APPLE__
#include <crt_externs.h>
#else
extern "C" char **environ;
#endif
#endif

#include "utils/printutils.h"

namespace Subprocess {

namespace {

#ifdef _WIN32

std::wstring widen(const std::string& utf8)
{
  if (utf8.empty()) return {};
  const int len =
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  std::wstring out(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), len);
  return out;
}

/*
   Quotes one argument for a Windows command line.

   Windows hands the process a single string and lets its runtime split it again, so
   the quoting rules have to be reproduced exactly: backslashes are only special in
   front of a quote, where they are doubled, including the run that ends the
   argument. Getting this wrong corrupts paths containing spaces or backslashes,
   which on Windows is most of them.
 */
std::string quoteArg(const std::string& arg)
{
  if (!arg.empty() && arg.find_first_of(" \t\"") == std::string::npos) return arg;

  std::string out = "\"";
  for (auto it = arg.begin();; ++it) {
    unsigned backslashes = 0;
    while (it != arg.end() && *it == '\\') {
      ++it;
      ++backslashes;
    }
    if (it == arg.end()) {
      out.append(backslashes * 2, '\\');
      break;
    }
    if (*it == '"') {
      out.append(backslashes * 2 + 1, '\\');
      out.push_back('"');
    } else {
      out.append(backslashes, '\\');
      out.push_back(*it);
    }
  }
  out.push_back('"');
  return out;
}

#endif  // _WIN32

}  // namespace

bool runAllAndWait(const std::vector<std::vector<std::string>>& commands)
{
  bool ok = true;

#ifdef _WIN32
  std::vector<HANDLE> handles;
  handles.reserve(commands.size());

  for (const auto& command : commands) {
    if (command.empty()) {
      ok = false;
      continue;
    }
    std::string line;
    for (const auto& arg : command) {
      if (!line.empty()) line.push_back(' ');
      line += quoteArg(arg);
    }
    std::wstring application = widen(command[0]);
    std::wstring commandLine = widen(line);

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(application.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        nullptr, &startupInfo, &processInfo)) {
      LOG(message_group::Error, "Can't start %1$s (error %2$lu).", command[0],
          static_cast<unsigned long>(GetLastError()));
      ok = false;
      continue;
    }
    CloseHandle(processInfo.hThread);
    handles.push_back(processInfo.hProcess);
  }

  // Wait for every child even after one has failed, so none are left running.
  for (const HANDLE handle : handles) {
    WaitForSingleObject(handle, INFINITE);
    DWORD exitCode = 1;
    if (!GetExitCodeProcess(handle, &exitCode) || exitCode != 0) {
      LOG(message_group::Error, "Worker process exited with status %1$lu.",
          static_cast<unsigned long>(exitCode));
      ok = false;
    }
    CloseHandle(handle);
  }
#else
  std::vector<pid_t> pids;
  pids.reserve(commands.size());

  for (const auto& command : commands) {
    if (command.empty()) {
      ok = false;
      continue;
    }
    std::vector<char *> argv;
    argv.reserve(command.size() + 1);
    for (const auto& arg : command) argv.push_back(const_cast<char *>(arg.c_str()));
    argv.push_back(nullptr);

#ifdef __APPLE__
    char **envp = *_NSGetEnviron();
#else
    char **envp = environ;
#endif
    pid_t pid = 0;
    const int rc = posix_spawn(&pid, command[0].c_str(), nullptr, nullptr, argv.data(), envp);
    if (rc != 0) {
      LOG(message_group::Error, "Can't start %1$s: %2$s.", command[0], strerror(rc));
      ok = false;
      continue;
    }
    pids.push_back(pid);
  }

  // Wait for every child even after one has failed, so none are left orphaned.
  for (const pid_t pid : pids) {
    int status = 0;
    bool waited = true;
    while (waitpid(pid, &status, 0) < 0) {
      if (errno != EINTR) {
        waited = false;
        break;
      }
    }
    if (!waited) {
      LOG(message_group::Error, "Can't wait for worker process: %1$s.", strerror(errno));
      ok = false;
    } else if (WIFSIGNALED(status)) {
      LOG(message_group::Error, "Worker process killed by signal %1$d.", WTERMSIG(status));
      ok = false;
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      LOG(message_group::Error, "Worker process exited with status %1$d.", WEXITSTATUS(status));
      ok = false;
    }
  }
#endif

  return ok;
}

}  // namespace Subprocess
