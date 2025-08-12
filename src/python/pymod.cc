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

#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

#include "core/Settings.h"
#include "platform/PlatformUtils.h"

#include "pyopenscad.h"

namespace fs = std::filesystem;

using SP = Settings::SettingsPython;

std::string venvBinDirFromSettings()
{
  const auto& venv = fs::path(SP::pythonVirtualEnv.value()) / "bin";
  if (fs::is_directory(venv)) {
    return venv.generic_string();
  }
  return "";
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

  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    goto fail;
  }
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
  int result = pythonRunModule("", "venv", {path});
  if (result != 0) {
    return result;
  }

  // The created VENV points to the temporary mount point of the
  // AppImage, e.g. /tmp/.mount_OpenSCCpPaio - that is obviously
  // no good for any later runs.
  // To fix that, we point the link to the magic /proc/self/exe
  // so it can always just call itself as the python interpreter.
  const char *appdirenv = getenv("APPDIR");
  if (getenv("APPIMAGE") != nullptr && appdirenv != nullptr) {
    // Assume we are running as AppImage
    const std::string appdir = appdirenv;
    const auto vbin = fs::path{path} / "bin" / "openscad-python";
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

int pythonRunModule(const std::string& appPath, const std::string& module,
                    const std::vector<std::string>& args)
{
  PyStatus status;
  const auto name = "openscad-python";
  const auto exe = PlatformUtils::applicationPath() + "/" + name;

  PyPreConfig preconfig;
  PyPreConfig_InitPythonConfig(&preconfig);

  status = Py_PreInitialize(&preconfig);
  if (PyStatus_Exception(status)) {
    Py_ExitStatusException(status);
  }

  PyConfig config;
  PyConfig_InitPythonConfig(&config);

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
    std::wstring warg(arg.size(), L' ');
    warg.resize(std::mbstowcs(&warg[0], arg.c_str(), arg.size()));
    status = PyWideStringList_Append(&config.argv, warg.c_str());
    if (PyStatus_Exception(status)) {
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

  return Py_RunMain();

done:
  PyConfig_Clear(&config);
  return status.exitcode;
}
