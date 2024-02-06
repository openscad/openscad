# If using mimalloc, the order of link libraries and sources on target OpenSCAD *might* make a difference,
#   in which case we want mimalloc to go first.
if(USE_MIMALLOC)
  target_compile_definitions(OpenSCAD PRIVATE USE_MIMALLOC) # For preprocessor checks in our source.

  get_property(OSCAD_LIBS TARGET OpenSCAD PROPERTY LINK_LIBRARIES)
  if(OSCAD_LIBS)
    message(AUTHOR_WARNING "Target OpenSCAD appears to have libraries linked before mimalloc (${OSCAD_LIBS}).  This might cause problems.")
  endif()

  find_package(mimalloc QUIET)
  if(TARGET mimalloc)
    # MSYS2 provides a mimalloc package with both static and shared builds
    message(STATUS "Using already installed mimalloc package, version ${mimalloc_VERSION}")
    target_compile_definitions(OpenSCAD PRIVATE MI_LINK_SHARED) # So cmake install/pack know which files are needed.
    target_link_libraries(OpenSCAD PRIVATE mimalloc)
  else()
    set(MI_SUBDIR mimalloc)
    set(mimalloc_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${MI_SUBDIR}/cmake" PARENT_SCOPE)

    set(MI_DEBUG_FULL        OFF CACHE BOOL "") # Use full internal heap invariant checking in DEBUG mode (expensive)
    set(MI_PADDING           ON  CACHE BOOL "") # Enable padding to detect heap block overflow (used only in DEBUG mode)
    if(APPLE)
      set(MI_OVERRIDE        OFF CACHE BOOL "") # Override the standard malloc interface (e.g. define entry points for malloc() etc)
    else()
      set(MI_OVERRIDE        ON  CACHE BOOL "")
    endif()
    set(MI_XMALLOC           OFF CACHE BOOL "") # Enable abort() call on memory allocation failure by default
    set(MI_SHOW_ERRORS       OFF CACHE BOOL "") # Show error and warning messages by default (only enabled by default in DEBUG mode)
    set(MI_USE_CXX           ON  CACHE BOOL "") # Use the C++ compiler to compile the library (instead of the C compiler)
    set(MI_SEE_ASM           OFF CACHE BOOL "") # Generate assembly files
    set(MI_INTERPOSE         ON  CACHE BOOL "") # Use interpose to override standard malloc on macOS
    set(MI_OSX_ZONE          ON  CACHE BOOL "") # Use malloc zone to override standard malloc on macOS
    set(MI_LOCAL_DYNAMIC_TLS OFF CACHE BOOL "") # Use slightly slower, dlopen-compatible TLS mechanism (Unix)
    set(MI_BUILD_SHARED      OFF CACHE BOOL "")
    set(MI_BUILD_STATIC      ON  CACHE BOOL "")
    set(MI_BUILD_OBJECT      OFF CACHE BOOL "")
    set(MI_BUILD_TESTS       OFF CACHE BOOL "") # Build test executables
    set(MI_DEBUG_TSAN        OFF CACHE BOOL "") # Build with thread sanitizer (needs clang)
    set(MI_DEBUG_UBSAN       OFF CACHE BOOL "") # Build with undefined-behavior sanitizer (needs clang++)
    set(MI_INSTALL_TOPLEVEL  OFF CACHE BOOL "") # Install directly into $CMAKE_INSTALL_PREFIX instead of PREFIX/lib/mimalloc-version

    target_include_directories(OpenSCAD SYSTEM PRIVATE ${MI_SUBDIR}/include)
    if(MI_BUILD_SHARED AND MXECROSS)
      set(CMAKE_SHARED_LIBRARY_PREFIX "") # avoid "lib" prefix
    endif()
    add_subdirectory(${MI_SUBDIR} EXCLUDE_FROM_ALL)

    if(MI_OVERRIDE)
      target_compile_definitions(OpenSCAD PRIVATE MI_OVERRIDE) # For preprocessor checks in our source.
    endif()

    # Only one MI_BUILD_[TYPE] is expected at one time
    if(MI_BUILD_SHARED)
      target_compile_definitions(OpenSCAD PRIVATE MI_LINK_SHARED)
      target_link_libraries(OpenSCAD PRIVATE mimalloc)
    elseif(MI_BUILD_STATIC)
      target_compile_definitions(OpenSCAD PRIVATE MI_LINK_STATIC)
      target_link_libraries(OpenSCAD PRIVATE mimalloc-static) # put before all other libraries
    elseif(MI_BUILD_OBJECT) # (for Unix-like systems only, according to mimalloc docs)
      target_compile_definitions(OpenSCAD PRIVATE MI_LINK_OBJECT)
      #   New in version 3.21.
      #   The object files associated with an object library may be referenced by the $<TARGET_OBJECTS> generator expression.
      #   Such object files are placed on the link line before all libraries, regardless of their relative order.
      #     from:  https://cmake.org/cmake/help/latest/command/target_link_libraries.html#linking-object-libraries-via-target-objects
      target_link_libraries(OpenSCAD PRIVATE $<TARGET_OBJECTS:mimalloc-obj> pthread)
    endif()
  endif()
endif()
