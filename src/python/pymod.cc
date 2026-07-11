/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2025 Clifford Wolf <clifford@clifford.at> and
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
#include <Python.h>

#include <array>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "core/Settings.h"
#ifndef PYTHON_EXECUTABLE_NAME
#include "executable.h"
#endif
#include "platform/PlatformUtils.h"
#include "pyopenscad.h"
#include "python_runtime.h"

namespace fs = std::filesystem;

using SP = Settings::SettingsPython;

std::string pythonShimExecutablePath()
{
  auto path = fs::path(PlatformUtils::applicationPath()) / PYTHON_EXECUTABLE_NAME;
#if defined(_WIN32)
  path += ".exe";
#endif
  return path.generic_string();
}

std::string venvBinDirFromSettings()
{
  const auto& venv = fs::path(SP::pythonVirtualEnv.value()) / "bin";
  if (fs::is_directory(venv)) {
    return venv.generic_string();
  }
  return "";
}

#if defined(_WIN32)
static std::string pythonWindowsRuntimePath()
{
  const auto applicationPath = fs::path(PlatformUtils::applicationPath());
  const auto pythonXY =
    "python" + std::to_string(PY_MAJOR_VERSION) + "." + std::to_string(PY_MINOR_VERSION);
  const auto windowsPythonLib = applicationPath / "lib" / pythonXY;
  const std::array<fs::path, 4> paths = {
    windowsPythonLib,
    windowsPythonLib / "lib-dynload",
    applicationPath,
    applicationPath / "DLLs",
  };

  std::ostringstream stream;
  std::string sep;
  for (const auto& path : paths) {
    std::error_code ec;
    if (!fs::is_directory(path, ec) || ec) continue;
    const auto absolutePath = fs::absolute(path, ec);
    if (ec) continue;
    stream << sep << absolutePath.generic_string();
    sep = ";";
  }
  return stream.str();
}

static void pythonConfigureHiddenConsoleSubprocesses()
{
  if (getenv("PYTHONSCAD_HIDE_CONSOLE_SUBPROCESSES") == nullptr) {
    return;
  }
  if (PyRun_SimpleString(R"PYTHON(
import subprocess as _pythonscad_subprocess
if not getattr(_pythonscad_subprocess, "_pythonscad_hidden_console", False):
    _pythonscad_original_popen = _pythonscad_subprocess.Popen
    class _PythonSCADHiddenPopen(_pythonscad_original_popen):
        def __init__(self, args, *popen_args, **kwargs):
            kwargs["creationflags"] = kwargs.get("creationflags", 0) | getattr(_pythonscad_subprocess, "CREATE_NO_WINDOW", 0)
            startupinfo = kwargs.get("startupinfo")
            if startupinfo is None:
                startupinfo = _pythonscad_subprocess.STARTUPINFO()
            startupinfo.dwFlags |= _pythonscad_subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = 0
            kwargs["startupinfo"] = startupinfo
            super().__init__(args, *popen_args, **kwargs)
    _pythonscad_subprocess.Popen = _PythonSCADHiddenPopen
    _pythonscad_subprocess._pythonscad_hidden_console = True
)PYTHON") != 0) {
    PyErr_Clear();
  }
}
#endif

#ifdef __EMSCRIPTEN__
// These functions manage Python runtime lifecycle for CLI / AppImage use cases.
// They are never called in WASM builds but must exist to satisfy the linker.
int pythonRunArgs(int, char **)
{
  __builtin_trap();
  __builtin_unreachable();
}
int pythonCreateVenv(const std::string&)
{
  __builtin_trap();
  __builtin_unreachable();
}
int pythonRunModule(const std::string&, const std::string&, const std::vector<std::string>&)
{
  __builtin_trap();
  __builtin_unreachable();
}
#else

static int pythonRunCommand(const std::string& command, const std::vector<std::string>& args);

static bool pythonAppendConfigArg(PyConfig& config, const std::string& arg, PyStatus& status)
{
  wchar_t *warg = Py_DecodeLocale(arg.c_str(), nullptr);
  if (warg == nullptr) {
    status = PyStatus_Error("failed to decode Python argument");
    return false;
  }
  status = PyWideStringList_Append(&config.argv, warg);
  PyMem_RawFree(warg);
  return !PyStatus_Exception(status);
}

int pythonRunArgs(int argc, char **argv)
{
  PyStatus status;

  PyConfig config;
  PyConfig_InitPythonConfig(&config);

  status = PyConfig_SetBytesArgv(&config, argc, argv);
  if (PyStatus_Exception(status)) {
    goto fail;
  }
#if defined(_WIN32)
  {
    const auto applicationPath = fs::path(PlatformUtils::applicationPath()).generic_string();
    status = PyConfig_SetBytesString(&config, &config.home, applicationPath.c_str());
    if (PyStatus_Exception(status)) {
      goto fail;
    }
    const auto pythonPath = pythonWindowsRuntimePath();
    if (!pythonPath.empty()) {
      status = PyConfig_SetBytesString(&config, &config.pythonpath_env, pythonPath.c_str());
      if (PyStatus_Exception(status)) {
        goto fail;
      }
    }
  }
#endif
  {
    const auto baseExecutable = pythonShimExecutablePath();
    std::error_code ec;
    if (fs::exists(baseExecutable, ec)) {
      status = PyConfig_SetBytesString(&config, &config.base_executable, baseExecutable.c_str());
      if (PyStatus_Exception(status)) {
        goto fail;
      }
    }
  }

  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    goto fail;
  }
#if defined(_WIN32)
  python_configure_windows_sys_compat();
  pythonConfigureHiddenConsoleSubprocesses();
#endif
  PyConfig_Clear(&config);

  return Py_RunMain();

fail:
  PyConfig_Clear(&config);
  if (PyStatus_IsExit(status)) {
    return status.exitcode;
  }
  Py_ExitStatusException(status);
}

int pythonCreateVenv(const std::string& path)
{
#if defined(_WIN32)
  static const char *const hiddenVenvCommand = R"PYTHON(
import os
import subprocess
import sys
import venv

class PythonSCADEnvBuilder(venv.EnvBuilder):
    def _call_new_python(self, context, *py_args, **kwargs):
        args = [context.env_exec_cmd, *py_args]
        kwargs["env"] = env = os.environ.copy()
        env["VIRTUAL_ENV"] = context.env_dir
        env["PYTHONSCAD_HIDE_CONSOLE_SUBPROCESSES"] = "1"
        env.pop("PYTHONHOME", None)
        env.pop("PYTHONPATH", None)
        kwargs["cwd"] = context.env_dir
        kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = 0
        kwargs["startupinfo"] = startupinfo
        subprocess.check_output(args, **kwargs)

PythonSCADEnvBuilder(with_pip=True, scm_ignore_files=frozenset(["git"])).create(sys.argv[1])
)PYTHON";
  int result = pythonRunCommand(hiddenVenvCommand, {path});
#else
  int result = pythonRunModule("", "venv", {path});
#endif
  if (result != 0) {
    return result;
  }

#if defined(_WIN32)
  {
    const auto scriptsDir = fs::path{path} / "Scripts";
    const auto pythonExe = scriptsDir / "python.exe";
    const auto pythonScadExe = scriptsDir / (std::string(PYTHON_EXECUTABLE_NAME) + ".exe");
    std::error_code ec;
    const bool pythonScadExists = fs::exists(pythonScadExe, ec);
    if (ec.value() > 0) {
      return ec.value();
    }
    const bool pythonExists = fs::exists(pythonExe, ec);
    if (ec.value() > 0) {
      return ec.value();
    }
    if (!pythonScadExists && pythonExists) {
      fs::copy_file(pythonExe, pythonScadExe, fs::copy_options::overwrite_existing, ec);
      if (ec.value() > 0) {
        return ec.value();
      }
    }
  }
#endif

  // The created VENV points to the temporary mount point of the
  // AppImage, e.g. /tmp/.mount_OpenSCCpPaio - that is obviously
  // no good for any later runs.
  // To fix that, we point the link to the magic /proc/self/exe
  // so it can always just call itself as the python interpreter.
  const char *appdirenv = getenv("APPDIR");
  if (getenv("APPIMAGE") != nullptr && appdirenv != nullptr) {
    // Assume we are running as AppImage
    const std::string appdir = appdirenv;
    const auto vbin = fs::path{path} / "bin" / PYTHON_EXECUTABLE_NAME;
    if (fs::exists(vbin) && fs::is_symlink(vbin)) {
      const auto lbin = fs::read_symlink(vbin).generic_string();
      if (lbin.rfind(appdir, 0) == 0) {
        std::error_code ec;
        fs::remove(vbin, ec);
        if (ec.value() > 0) {
          return ec.value();
        }
        fs::create_symlink("/proc/self/exe", vbin, ec);
        if (ec.value() > 0) {
          return ec.value();
        }
      }
    }
  }

  return 0;
}

static int pythonRunCommand(const std::string& command, const std::vector<std::string>& args)
{
  PyStatus status;
  const auto name = PYTHON_EXECUTABLE_NAME;
  const auto exe = pythonShimExecutablePath();

  PyPreConfig preconfig;
  PyPreConfig_InitPythonConfig(&preconfig);

  status = Py_PreInitialize(&preconfig);
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }

  PyConfig config;
  PyConfig_InitPythonConfig(&config);
#if defined(_WIN32)
  {
    const auto applicationPath = fs::path(PlatformUtils::applicationPath()).generic_string();
    status = PyConfig_SetBytesString(&config, &config.home, applicationPath.c_str());
    if (PyStatus_Exception(status)) {
      goto done;
    }
    const auto pythonPath = pythonWindowsRuntimePath();
    if (!pythonPath.empty()) {
      status = PyConfig_SetBytesString(&config, &config.pythonpath_env, pythonPath.c_str());
      if (PyStatus_Exception(status)) {
        goto done;
      }
    }
  }
#endif
  {
    const auto baseExecutable = pythonShimExecutablePath();
    std::error_code ec;
    if (fs::exists(baseExecutable, ec)) {
      status = PyConfig_SetBytesString(&config, &config.base_executable, baseExecutable.c_str());
      if (PyStatus_Exception(status)) {
        goto done;
      }
    }
  }

  status = PyConfig_SetBytesString(&config, &config.program_name, name);
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = PyConfig_SetBytesString(&config, &config.executable, exe.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = PyConfig_SetBytesString(&config, &config.run_command, command.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = PyConfig_Read(&config);
  if (PyStatus_Exception(status)) {
    goto done;
  }

  for (const auto& arg : args) {
    if (!pythonAppendConfigArg(config, arg, status)) {
      goto done;
    }
  }

  status = PyConfig_SetBytesString(&config, &config.executable, exe.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    goto done;
  }
#if defined(_WIN32)
  python_configure_windows_sys_compat();
  pythonConfigureHiddenConsoleSubprocesses();
#endif

  PyConfig_Clear(&config);
  return Py_RunMain();

done:
  PyConfig_Clear(&config);
  if (!PyStatus_IsExit(status) && PyStatus_Exception(status)) {
    return 1;
  }
  return status.exitcode;
}

int pythonRunModule(const std::string& appPath, const std::string& module,
                    const std::vector<std::string>& args)
{
  PyStatus status;
  const auto name = PYTHON_EXECUTABLE_NAME;
  const auto exe = pythonShimExecutablePath();

  PyPreConfig preconfig;
  PyPreConfig_InitPythonConfig(&preconfig);

  status = Py_PreInitialize(&preconfig);
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }

  PyConfig config;
  PyConfig_InitPythonConfig(&config);
#if defined(_WIN32)
  {
    const auto applicationPath = fs::path(PlatformUtils::applicationPath()).generic_string();
    status = PyConfig_SetBytesString(&config, &config.home, applicationPath.c_str());
    if (PyStatus_Exception(status)) {
      goto done;
    }
    const auto pythonPath = pythonWindowsRuntimePath();
    if (!pythonPath.empty()) {
      status = PyConfig_SetBytesString(&config, &config.pythonpath_env, pythonPath.c_str());
      if (PyStatus_Exception(status)) {
        goto done;
      }
    }
  }
#endif
  {
    const auto baseExecutable = pythonShimExecutablePath();
    std::error_code ec;
    if (fs::exists(baseExecutable, ec)) {
      status = PyConfig_SetBytesString(&config, &config.base_executable, baseExecutable.c_str());
      if (PyStatus_Exception(status)) {
        goto done;
      }
    }
  }

  status = PyConfig_SetBytesString(&config, &config.program_name, name);
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = PyConfig_SetBytesString(&config, &config.executable, exe.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = PyConfig_SetBytesString(&config, &config.run_module, module.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  /* Read all configuration at once */
  status = PyConfig_Read(&config);
  if (PyStatus_Exception(status)) {
    goto done;
  }

  for (const auto& arg : args) {
    if (!pythonAppendConfigArg(config, arg, status)) {
      goto done;
    }
  }

  /* Override executable computed by PyConfig_Read() */
  status = PyConfig_SetBytesString(&config, &config.executable, exe.c_str());
  if (PyStatus_Exception(status)) {
    goto done;
  }

  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    goto done;
  }
#if defined(_WIN32)
  python_configure_windows_sys_compat();
  pythonConfigureHiddenConsoleSubprocesses();
#endif

  PyConfig_Clear(&config);
  return Py_RunMain();

done:
  PyConfig_Clear(&config);
  if (!PyStatus_IsExit(status) && PyStatus_Exception(status)) {
    return 1;
  }
  return status.exitcode;
}

#endif  // !__EMSCRIPTEN__
