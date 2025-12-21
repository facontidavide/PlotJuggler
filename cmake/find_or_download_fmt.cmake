function(find_or_download_fmt)

  find_package(fmt QUIET)

  if(fmt_FOUND)
    message(STATUS "Found fmt in system")
  elseif(NOT TARGET fmt)
    message(STATUS "fmt not found, downloading")

    cpmaddpackage(NAME fmt URL
                  https://github.com/fmtlib/fmt/archive/refs/tags/11.2.0.zip)

    set(fmt_FOUND TRUE FORCE)
    add_library(fmt::fmt ALIAS fmt)
  endif()

endfunction()
