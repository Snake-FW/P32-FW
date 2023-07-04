macro(add_c_and_cxx_compile_options NAME)
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:${NAME}>")
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${NAME}>")
endmacro()

macro(add_c_and_cxx_compile_definitions NAME)
  add_compile_definitions("$<$<COMPILE_LANGUAGE:C>:${NAME}>")
  add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:${NAME}>")
endmacro()

if(MSVC)
  add_c_and_cxx_compile_options("/W4")
  add_c_and_cxx_compile_options("/WX")
else()
  add_c_and_cxx_compile_options("-Wall")
  add_c_and_cxx_compile_options("-Wextra")
  add_c_and_cxx_compile_options("-pedantic")
  add_c_and_cxx_compile_options("-Wshadow")
  add_c_and_cxx_compile_options("-Wconversion")
  add_c_and_cxx_compile_options("-Werror")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_c_and_cxx_compile_definitions("_LIBCPP_ENABLE_NODISCARD")

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0)
      add_c_and_cxx_compile_options("-Wno-missing-braces")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_c_and_cxx_compile_definitions("_LIBCPP_ENABLE_NODISCARD")
    add_c_and_cxx_compile_options("-Wno-missing-braces")
  endif()
endif()