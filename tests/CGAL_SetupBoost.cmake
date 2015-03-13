if ( NOT CGAL_Boost_Setup )

  include(CGAL_TweakFindBoost)
  # In the documentation, we say we require Boost-1.39, but technically we
  # require 1.33.1. Some packages may require more recent versions, though.
  #find_package( Boost 1.33.1 REQUIRED thread system )
  find_package( Boost 1.33.1 OPTIONAL_COMPONENTS thread system )

  if(Boost_FOUND)
    if(DEFINED Boost_DIR AND NOT Boost_DIR)
      # Unset that cache variable that is set in the cache by FindBoost
      # (while it was searching for boost-cmake).
      unset(Boost_DIR CACHE)
      set(Boost_NO_BOOST_CMAKE TRUE CACHE INTERNAL "Avoid future search of boost-cmake")
    endif()
  endif()
  
  message( STATUS "Boost include:     ${Boost_INCLUDE_DIRS}" )
  message( STATUS "Boost libraries:   ${Boost_LIBRARIES}" )
  message( STATUS "Boost definitions: ${Boost_DEFINITIONS}" )
  
  set ( CGAL_USE_BOOST 1 )
  
  include(CGAL_Macros)
  
  add_to_cached_list(CGAL_3RD_PARTY_INCLUDE_DIRS   ${Boost_INCLUDE_DIRS} )
  add_to_cached_list(CGAL_3RD_PARTY_LIBRARIES_DIRS ${Boost_LIBRARY_DIRS} )
  add_to_cached_list(CGAL_3RD_PARTY_DEFINITIONS    ${Boost_DEFINITIONS}  )
  
  if ( NOT MSVC )
    add_to_cached_list(CGAL_3RD_PARTY_LIBRARIES ${Boost_LIBRARIES} )
  endif()
  
  message( STATUS "USING BOOST_VERSION = '${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}'" )
  
  set ( CGAL_Boost_Setup TRUE )
  
endif()

