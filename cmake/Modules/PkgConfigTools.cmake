# Use this to save the existing pkg-config settings
function(save_pkg_config_env)
  message(STATUS "saving pkg-config env")

  if (DEFINED ENV{PKG_CONFIG_PATH})
    set(SAVED_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}")
  endif()
  if (DEFINED ENV{PKG_CONFIG_LIBDIR})
    set(SAVED_PKG_CONFIG_LIBDIR "$ENV{PKG_CONFIG_LIBDIR}")
  endif()
endfunction()

# Use this to restore to the original pkg-config settings
function(restore_pkg_config_env)
  message(STATUS "restoring pkg-config env")

  if (SAVED_PKG_CONFIG_PATH)
    set(ENV{PKG_CONFIG_PATH} "${SAVED_PKG_CONFIG_PATH}")
    unset(SAVED_PKG_CONFIG_PATH)
  else()
    unset(ENV{PKG_CONFIG_PATH})
  endif()
  if (SAVED_PKG_CONFIG_LIBDIR)
    set(ENV{PKG_CONFIG_LIBDIR} "${SAVED_PKG_CONFIG_LIBDIR}")
    unset(SAVED_PKG_CONFIG_LIBDIR)
  else()
    unset(ENV{PKG_CONFIG_LIBDIR})
  endif()
endfunction()
