#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "json-c::json-c" for configuration "Debug"
set_property(TARGET json-c::json-c APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(json-c::json-c PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/json-c.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/json-c.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS json-c::json-c )
list(APPEND _IMPORT_CHECK_FILES_FOR_json-c::json-c "${_IMPORT_PREFIX}/lib/json-c.lib" "${_IMPORT_PREFIX}/bin/json-c.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
