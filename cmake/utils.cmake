function(util_add_lib_subdirectory libname)
  add_subdirectory(${BASE_PROJECT_SOURCE_DIR}/libs/${libname} ${CMAKE_BINARY_DIR}/libs/${libname} EXCLUDE_FROM_ALL)
endfunction()