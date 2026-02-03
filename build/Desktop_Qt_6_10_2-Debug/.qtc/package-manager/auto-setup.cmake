#
# Internal Qt Creator variable reference
#
foreach(qtcreator_var
    QT_QMAKE_EXECUTABLE CMAKE_PREFIX_PATH CMAKE_C_COMPILER CMAKE_CXX_COMPILER
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  set(__just_reference_${qtcreator_var} ${${qtcreator_var}})
endforeach()

option(QT_CREATOR_SOURCE_GROUPS "Qt Creator source groups extensions" ON)
if (QT_CREATOR_SOURCE_GROUPS)
  source_group("Resources" REGULAR_EXPRESSION "\\.(pdf|plist|png|jpeg|jpg|storyboard|xcassets|qrc|svg|gif|ico|webp)$")
  source_group("Forms" REGULAR_EXPRESSION "\\.(ui)$")
  source_group("State charts" REGULAR_EXPRESSION "\\.(scxml)$")
  source_group("Source Files" REGULAR_EXPRESSION
    "\\.(C|F|M|c|c\\+\\+|cc|cpp|mpp|cxx|ixx|cppm|ccm|cxxm|c\\+\\+m|cu|f|f90|for|fpp|ftn|m|mm|rc|def|r|odl|idl|hpj|bat|qml|js)$"
  )
endif()

#
# Set a better default value for CMAKE_INSTALL_PREFIX
#
function(qtc_modify_default_install_prefix)
  # If at configure time the user didn't specify a CMAKE_INSTALL_PREFIX variable
  # Modules/CMakeGenericSystem.cmake will set a default value
  # to CMAKE_INSTALL_PREFIX and set CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT to ON

  # In practice there are cases when CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT is
  # set to ON and a custom CMAKE_INSTALL_PREFIX is set

  # Do the original CMAKE_INSTALL_PREFIX detection
  if(CMAKE_HOST_UNIX)
    set(original_cmake_install_prefix "/usr/local")
  else()
    GetDefaultWindowsPrefixBase(CMAKE_GENERIC_PROGRAM_FILES)
    set(original_cmake_install_prefix
      "${CMAKE_GENERIC_PROGRAM_FILES}/${PROJECT_NAME}")
    unset(CMAKE_GENERIC_PROGRAM_FILES)
  endif()

  # When the user code didn't modify the CMake set CMAKE_INSTALL_PREFIX
  # then set the "/tmp" better value for CMAKE_INSTALL_PREFIX
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND
      CMAKE_INSTALL_PREFIX STREQUAL "${original_cmake_install_prefix}")
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/tmp")
  endif()
endfunction()
if (CMAKE_VERSION GREATER_EQUAL "3.19")
  cmake_language(DEFER CALL qtc_modify_default_install_prefix)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
  include("${CMAKE_SOURCE_DIR}/QtCreatorPackageManager.cmake")
endif()

if (QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP)
  return()
endif()
option(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP "Skip Qt Creator's package manager auto-setup" OFF)

# Store the C/C++ object output extension
if (CMAKE_VERSION GREATER_EQUAL "3.19")
  cmake_language(DEFER CALL set CMAKE_C_OUTPUT_EXTENSION "${CMAKE_C_OUTPUT_EXTENSION}" CACHE STRING "" FORCE)
  cmake_language(DEFER CALL set CMAKE_CXX_OUTPUT_EXTENSION "${CMAKE_CXX_OUTPUT_EXTENSION}" CACHE STRING "" FORCE)
endif()

macro(qtc_auto_setup_compiler_standard toolchainFile)
  foreach(lang_var C CXX CUDA OBJC OBJCXX)
    foreach(prop_var STANDARD STANDARD_REQUIRED EXTENSIONS)
      if (CMAKE_${lang_var}_${prop_var})
        file(APPEND "${toolchainFile}"
             "set(CMAKE_${lang_var}_${prop_var} ${CMAKE_${lang_var}_${prop_var}})\n")
      endif()
    endforeach()
  endforeach()

  # Forward important CMake variables to the package manager in the toolchain file
  foreach(fwd_var CMAKE_MSVC_RUNTIME_LIBRARY CMAKE_SYSROOT
                  CMAKE_OSX_SYSROOT CMAKE_OSX_ARCHITECTURES
                  ANDROID_ABI ANDROID_NDK)
    if (${fwd_var})
      file(APPEND "${toolchainFile}"
          "set(${fwd_var} \"${${fwd_var}}\")\n")
    endif()
  endforeach()
endmacro()

#
# conan
#
macro(qtc_auto_setup_conan)
  foreach(file conanfile.txt conanfile.py)
    if (EXISTS "${CMAKE_SOURCE_DIR}/${file}")
      set(conanfile_txt "${CMAKE_SOURCE_DIR}/${file}")
      break()
    endif()
  endforeach()

  if (conanfile_txt AND NOT QT_CREATOR_SKIP_CONAN_SETUP)
    option(QT_CREATOR_SKIP_CONAN_SETUP "Skip Qt Creator's conan package manager auto-setup" OFF)
    set(QT_CREATOR_CONAN_BUILD_POLICY "missing" CACHE STRING "Qt Creator's conan package manager auto-setup build policy. This is used for the BUILD property of cmake_conan_run")

    set_property(
      DIRECTORY "${CMAKE_SOURCE_DIR}"
      APPEND
      PROPERTY CMAKE_CONFIGURE_DEPENDS "${conanfile_txt}")

    find_program(conan_program conan)
    if (NOT conan_program)
      message(WARNING "Qt Creator: conan executable not found. "
                      "Package manager auto-setup will be skipped. "
                      "To disable this warning set QT_CREATOR_SKIP_CONAN_SETUP to ON.")
      return()
    endif()
    execute_process(COMMAND ${conan_program} --version
      RESULT_VARIABLE result_code
      OUTPUT_VARIABLE conan_version_output
      ERROR_VARIABLE conan_version_output)
    if (NOT result_code EQUAL 0)
      message(FATAL_ERROR "conan --version failed='${result_code}: ${conan_version_output}")
    endif()

    string(REGEX REPLACE ".*Conan version ([0-9].[0-9]).*" "\\1" conan_version "${conan_version_output}")

    set(conanfile_timestamp_file "${CMAKE_BINARY_DIR}/conan-dependencies/conanfile.timestamp")
    file(TIMESTAMP "${conanfile_txt}" conanfile_timestamp)

    set(do_conan_installation ON)
    if (EXISTS "${conanfile_timestamp_file}")
      file(READ "${conanfile_timestamp_file}" old_conanfile_timestamp)
      if ("${conanfile_timestamp}" STREQUAL "${old_conanfile_timestamp}")
        set(do_conan_installation OFF)
      endif()
    endif()

    set(conanfile_build_policy_file "${CMAKE_BINARY_DIR}/conan-dependencies/conanfile.buildpolicy")
    if (EXISTS "${conanfile_build_policy_file}")
      file(READ "${conanfile_build_policy_file}" build_policy)
      if (NOT "${build_policy}" STREQUAL "${QT_CREATOR_CONAN_BUILD_POLICY}")
        set(do_conan_installation ON)
      endif()
    endif()

    if (do_conan_installation)
      message(STATUS "Qt Creator: conan package manager auto-setup. "
                     "Skip this step by setting QT_CREATOR_SKIP_CONAN_SETUP to ON.")

      file(COPY "${conanfile_txt}" DESTINATION "${CMAKE_BINARY_DIR}/conan-dependencies/")

      # conanfile should have a generator specified, when both file and conan_install
      # specifcy the CMakeDeps generator, conan_install will issue an error
      file(READ "${conanfile_txt}" conanfile_text_content)
      unset(conan_generator)
      if (NOT "${conanfile_text_content}" MATCHES ".*CMakeDeps.*")
        set(conan_generator "-g CMakeDeps")
      endif()

      file(WRITE "${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake" "
        set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\")
        set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\")
        ")
      qtc_auto_setup_compiler_standard("${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake")

      if (CMAKE_TOOLCHAIN_FILE)
        file(APPEND "${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake"
          "include(\"${CMAKE_TOOLCHAIN_FILE}\")\n")
      endif()

      file(WRITE "${CMAKE_BINARY_DIR}/conan-dependencies/CMakeLists.txt" "
        cmake_minimum_required(VERSION 3.15)

        unset(CMAKE_PROJECT_INCLUDE_BEFORE CACHE)
        project(conan-setup)

        if (${conan_version} VERSION_GREATER_EQUAL 2.0)
          set(CONAN_COMMAND \"${conan_program}\")
          include(\"${CMAKE_CURRENT_LIST_DIR}/conan_provider.cmake\")
          conan_profile_detect_default()
          detect_host_profile(\"${CMAKE_BINARY_DIR}/conan-dependencies/conan_host_profile\")

          set(build_types \${CMAKE_BUILD_TYPE})
          if (CMAKE_CONFIGURATION_TYPES)
            set(build_types \${CMAKE_CONFIGURATION_TYPES})
          endif()

          foreach(type \${build_types})
            conan_install(
              -pr \"${CMAKE_BINARY_DIR}/conan-dependencies/conan_host_profile\"
              --build=${QT_CREATOR_CONAN_BUILD_POLICY}
              -s build_type=\${type}
              ${conan_generator})
          endforeach()

          get_property(CONAN_INSTALL_SUCCESS GLOBAL PROPERTY CONAN_INSTALL_SUCCESS)
          if (CONAN_INSTALL_SUCCESS)
            get_property(CONAN_GENERATORS_FOLDER GLOBAL PROPERTY CONAN_GENERATORS_FOLDER)
            file(TO_CMAKE_PATH \"\${CONAN_GENERATORS_FOLDER}\" CONAN_GENERATORS_FOLDER)
            file(WRITE \"${CMAKE_BINARY_DIR}/conan-dependencies/conan_paths.cmake\" \"
              list(PREPEND CMAKE_PREFIX_PATH \\\"\${CONAN_GENERATORS_FOLDER}\\\")
              list(PREPEND CMAKE_MODULE_PATH \\\"\${CONAN_GENERATORS_FOLDER}\\\")
              list(PREPEND CMAKE_FIND_ROOT_PATH \\\"\${CONAN_GENERATORS_FOLDER}\\\")
              list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
              list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
              list(REMOVE_DUPLICATES CMAKE_FIND_ROOT_PATH)
              set(CMAKE_PREFIX_PATH \\\"\\\${CMAKE_PREFIX_PATH}\\\" CACHE STRING \\\"\\\" FORCE)
              set(CMAKE_MODULE_PATH \\\"\\\${CMAKE_MODULE_PATH}\\\" CACHE STRING \\\"\\\" FORCE)
              set(CMAKE_FIND_ROOT_PATH \\\"\\\${CMAKE_FIND_ROOT_PATH}\\\" CACHE STRING \\\"\\\" FORCE)
            \")
          endif()
        else()
          include(\"${CMAKE_CURRENT_LIST_DIR}/conan.cmake\")
          conan_cmake_run(
            CONANFILE \"${conanfile_txt}\"
            INSTALL_FOLDER \"${CMAKE_BINARY_DIR}/conan-dependencies\"
            GENERATORS cmake_paths cmake_find_package json
            BUILD ${QT_CREATOR_CONAN_BUILD_POLICY}
            ENV CONAN_CMAKE_TOOLCHAIN_FILE=\"${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake\"
          )
        endif()
      ")

      if (NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
          set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
      endif()

      execute_process(COMMAND ${CMAKE_COMMAND}
        -S "${CMAKE_BINARY_DIR}/conan-dependencies/"
        -B "${CMAKE_BINARY_DIR}/conan-dependencies/build"
        -C "${CMAKE_BINARY_DIR}/qtcsettings.cmake"
        -D "CMAKE_TOOLCHAIN_FILE=${CMAKE_BINARY_DIR}/conan-dependencies/toolchain.cmake"
        -G ${CMAKE_GENERATOR}
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D "CMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}"
        RESULT_VARIABLE result
      )
      if (result EQUAL 0)
        file(WRITE "${conanfile_timestamp_file}" "${conanfile_timestamp}")
        file(WRITE "${conanfile_build_policy_file}" ${QT_CREATOR_CONAN_BUILD_POLICY})
      else()
        message(WARNING "Qt Creator's conan package manager auto-setup failed. Consider setting "
                        "QT_CREATOR_SKIP_CONAN_SETUP to ON and reconfigure to skip this step.")
        return()
      endif()
    endif()

    include("${CMAKE_BINARY_DIR}/conan-dependencies/conan_paths.cmake")
  endif()
  unset(conanfile_txt)
endmacro()
qtc_auto_setup_conan()

#
# vcpkg
#
macro(qtc_auto_setup_vcpkg)
  if (EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json" AND NOT QT_CREATOR_SKIP_VCPKG_SETUP)
    option(QT_CREATOR_SKIP_VCPKG_SETUP "Skip Qt Creator's vcpkg package manager auto-setup" OFF)

    set_property(
      DIRECTORY "${CMAKE_SOURCE_DIR}"
      APPEND
      PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/vcpkg.json")

    find_program(vcpkg_program vcpkg
      PATHS ${CMAKE_SOURCE_DIR}/vcpkg ${CMAKE_SOURCE_DIR}/3rdparty/vcpkg $ENV{VCPKG_ROOT}
      NO_DEFAULT_PATH
    )
    if (NOT vcpkg_program)
      message(WARNING "Qt Creator: vcpkg executable not found. "
                      "Package manager auto-setup will be skipped. "
                      "To disable this warning set QT_CREATOR_SKIP_VCPKG_SETUP to ON.")
      return()
    endif()
    execute_process(COMMAND ${vcpkg_program} version
      RESULT_VARIABLE result_code
      OUTPUT_VARIABLE vcpkg_version_output
      ERROR_VARIABLE vcpkg_version_output)
    if (NOT result_code EQUAL 0)
      message(FATAL_ERROR "vcpkg version failed='${result_code}: ${vcpkg_version_output}")
    endif()

    # Resolve any symlinks
    get_filename_component(vpkg_program_real_path ${vcpkg_program} REALPATH)
    get_filename_component(vpkg_root ${vpkg_program_real_path} DIRECTORY)

    if (NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")
      message(STATUS "Qt Creator: vcpkg package manager auto-setup. "
                     "Skip this step by setting QT_CREATOR_SKIP_VCPKG_SETUP to ON.")

      file(WRITE "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" "
        set(CMAKE_C_COMPILER \"${CMAKE_C_COMPILER}\")
        set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\")
        ")
      qtc_auto_setup_compiler_standard("${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")

      # If set explicitly in Qt Creator's CMake parameters, just set it in the toolchain
      if (VCPKG_TARGET_TRIPLET)
        file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake"
          "set(VCPKG_TARGET_TRIPLET \"${VCPKG_TARGET_TRIPLET}\")\n")
      endif()

      if (CMAKE_TOOLCHAIN_FILE AND NOT
          CMAKE_TOOLCHAIN_FILE STREQUAL "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake")
        file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake"
          "include(\"${CMAKE_TOOLCHAIN_FILE}\")\n")
      endif()

      file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" [=[
        if (VCPKG_TARGET_TRIPLET)
          set(vcpkg_triplet ${VCPKG_TARGET_TRIPLET})
        else()
          if (ANDROID_ABI)
            if (ANDROID_ABI STREQUAL "armeabi-v7a")
              set(vcpkg_triplet arm-neon-android)
            elseif (ANDROID_ABI STREQUAL "arm64-v8a")
              set(vcpkg_triplet arm64-android)
            elseif (ANDROID_ABI STREQUAL "x86")
                set(vcpkg_triplet x86-android)
            elseif (ANDROID_ABI STREQUAL "x86_64")
                set(vcpkg_triplet x64-android)
            else()
                message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
            endif()
            set(ENV{ANDROID_NDK_HOME} "${ANDROID_NDK}")
          elseif (WIN32)
            if ("$ENV{PROCESSOR_ARCHITECTURE}" STREQUAL "ARM64")
              set(vcpkg_triplet arm64-mingw-static)
            else()
              set(vcpkg_triplet x64-mingw-static)
            endif()
            if (CMAKE_CXX_COMPILER MATCHES ".*/(.*)/cl.exe")
              string(TOLOWER ${CMAKE_MATCH_1} host_arch_lowercase)
              set(vcpkg_triplet ${host_arch_lowercase}-windows)
            endif()
          elseif(APPLE)
            # We're too early to use CMAKE_HOST_SYSTEM_PROCESSOR
            execute_process(
              COMMAND uname -m
              OUTPUT_VARIABLE __apple_host_system_processor
              OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (__apple_host_system_processor MATCHES "arm64")
              set(vcpkg_triplet arm64-osx)
            else()
              set(vcpkg_triplet x64-osx)
            endif()
          else()
            # We're too early to use CMAKE_HOST_SYSTEM_PROCESSOR
            execute_process(
              COMMAND uname -m
              OUTPUT_VARIABLE __linux_host_system_processor
              OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (__linux_host_system_processor MATCHES "aarch64")
              set(vcpkg_triplet arm64-linux)
            else()
              set(vcpkg_triplet x64-linux)
            endif()
          endif()
        endif()

        set(VCPKG_TARGET_TRIPLET ${vcpkg_triplet})
      ]=])

      file(APPEND "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" "
        include(\"${vpkg_root}/scripts/buildsystems/vcpkg.cmake\")
      ")
    endif()

    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/vcpkg-dependencies/toolchain.cmake" CACHE PATH "" FORCE)

    # Save CMAKE_PREFIX_PATH and CMAKE_MODULE_PATH as cache variables
    if (CMAKE_VERSION GREATER_EQUAL "3.19")
      cmake_language(DEFER CALL list REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
      cmake_language(DEFER CALL list REMOVE_DUPLICATES CMAKE_MODULE_PATH)
      cmake_language(DEFER CALL set CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "" FORCE)
      cmake_language(DEFER CALL set CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" CACHE STRING "" FORCE)
    endif()
  endif()
endmacro()
qtc_auto_setup_vcpkg()

#
# MaintenanceTool
#
if (QT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER)
  return()
endif()

option(QT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER
       "Skip Qt Creator's MaintenanceTool find_package provider" OFF)
option(QT_CREATOR_MAINTENANCE_TOOL_PROVIDER_USE_CLI
       "Use CLI mode for Qt Creator's MaintenanceTool find_package provider" OFF)

function(qtc_maintenance_provider_missing_variable_message variable)
  message(STATUS "Qt Creator: ${variable} was not set. "
                 "Qt MaintenanceTool cannot be used to install missing Qt modules that you specify in find_package(). "
                 "To disable this message set QT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER to ON.")
endfunction()

if (NOT QT_MAINTENANCE_TOOL)
  qtc_maintenance_provider_missing_variable_message(QT_MAINTENANCE_TOOL)
  return()
endif()
if (NOT QT_QMAKE_EXECUTABLE)
  qtc_maintenance_provider_missing_variable_message(QT_QMAKE_EXECUTABLE)
  return()
endif()

if (CMAKE_VERSION GREATER_EQUAL "3.24")
  list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/maintenance_tool_provider.cmake)
else()
  message(WARNING "Qt Creator: CMake version 3.24 is needed for MaintenanceTool find_package() provider. "
                  "To disable this warning set QT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER to ON.")
endif()
