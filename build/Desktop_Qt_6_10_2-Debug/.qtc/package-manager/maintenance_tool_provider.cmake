function(qt_maintenance_tool_get_component_platform platform_dir component_platform)
  # Mapping between platform file directory and component platform
  set(map_llvm-mingw_64 win64_llvm_mingw)
  set(map_mingw_64 win64_mingw)
  set(map_msvc2019_64 win64_msvc2019_64)
  set(map_msvc2022_64 win64_msvc2022_64)
  set(map_msvc2022_arm64 win64_msvc2022_arm64)
  set(map_gcc_64 linux_gcc_64)
  set(map_gcc_arm64 linux_gcc_arm64)
  set(map_ios ios)
  set(map_macos clang_64)
  set(map_android_arm64_v8a android)
  set(map_android_armv7 android)
  set(map_android_x86 android)
  set(map_android_x86_64 android)
  set(map_wasm_multithread wasm_multithread)
  set(map_wasm_singlethread wasm_singlethread)

  if (platform_dir STREQUAL "msvc2022_arm64" AND QT_HOST_PATH)
    set(${component_platform} "win64_msvc2022_arm64_cross_compiled" PARENT_SCOPE)
    return()
  endif()

  set(${component_platform} ${map_${platform_dir}} PARENT_SCOPE)
endfunction()

function(qt_maintenance_tool_get_addons addon_list)
  set(${addon_list}
    qt3d
    qt5compat
    qtcharts
    qtconnectivity
    qtdatavis3d
    qtgraphs
    qtgrpc
    qthttpserver
    qtimageformats
    qtlocation
    qtlottie
    qtmultimedia
    qtnetworkauth
    qtpositioning
    qtquick3d
    qtquick3dphysics
    qtquickeffectmaker
    qtquicktimeline
    qtremoteobjects
    qtscxml
    qtsensors
    qtserialbus
    qtserialport
    qtshadertools
    qtspeech
    qtvirtualkeyboard
    qtwebchannel
    qtwebsockets
    qtwebview

    # found in commercial version
    qtapplicationmanager
    qtinterfraceframework
    qtlanguageserver
    qtmqtt
    qtstatemachine
    qtopcua
    tqtc-qtvncserver

    PARENT_SCOPE
  )
endfunction()

function(qt_maintenance_tool_get_extensions extensions)
  set(${extensions}
    qtinsighttracker
    qtpdf
    qtwebengine

    PARENT_SCOPE
  )
endfunction()

function(qt_maintenance_tool_get_standalone_addons standalone_addons_list)
  set(${standalone_addons_list}
    qtquick3d
    qt5compat
    qtshadertools
    qtquicktimeline

    PARENT_SCOPE
  )
endfunction()

function(qt_maintenance_tool_remove_installed_components components_list)
  set(actual_components_list ${${components_list}})
  execute_process(
    COMMAND "${QT_MAINTENANCE_TOOL}" list
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    TIMEOUT 600
  )
  if (NOT result EQUAL 0)
    message(WARNING "Qt MaintenanceTool returned an error.\n${output}")
    set(${components_list} "" PARENT_SCOPE)
    return()
  endif()

  foreach(component_name IN LISTS actual_components_list)
    string(FIND "${output}" "<package name=\"${component_name}\"" found_component)
    if (NOT found_component EQUAL -1)
      list(REMOVE_ITEM ${components_list} ${component_name})
    endif()
  endforeach()
  set(${components_list} ${${components_list}} PARENT_SCOPE)
endfunction()

function(qt_maintenance_tool_install qt_major_version qt_package_list)
  if (QT_QMAKE_EXECUTABLE MATCHES ".*/(.*)/(.*)/bin/qmake")
    set(qt_version_number ${CMAKE_MATCH_1})
    string(REPLACE "." "" qt_version_number_dotless ${qt_version_number})
    set(qt_build_flavor ${CMAKE_MATCH_2})

    set(additional_addons "")
    qt_maintenance_tool_get_extensions(__qt_extensions)
    if (qt_version_number VERSION_LESS 6.8.0)
      set(additional_addons ${__qt_extensions})
    endif()
    if (WIN32)
      list(APPEND additional_addons qtactiveqt)
    endif()
    if (UNIX AND NOT APPLE)
      list(APPEND additional_addons qtwayland)
    endif()

    qt_maintenance_tool_get_component_platform(${qt_build_flavor} component_platform)
    if (NOT component_platform)
      message(STATUS
        "Qt Creator: Could not find the component platform for '${qt_build_flavor}' "
        "required by the MaintenanceTool. This can happen with a non Qt SDK installation "
        "(e.g. system Linux or macOS homebrew).")
      return()
    endif()

    set(installer_component_list "")
    foreach (qt_package_name IN LISTS qt_package_list)
      string(TOLOWER "${qt_package_name}" qt_package_name_lowercase)

      qt_maintenance_tool_get_addons(__qt_addons)
      if (qt_version_number VERSION_LESS 6.8.0)
        qt_maintenance_tool_get_standalone_addons(__standalone_addons)
        foreach(standalone_addon IN LISTS __standalone_addons)
          list(REMOVE_ITEM __qt_addons ${standalone_addon})
        endforeach()
      endif()

      # Is the package an addon?
      set(install_addon FALSE)
      foreach(addon IN LISTS __qt_addons additional_addons)
        string(REGEX MATCH "^${addon}$" is_addon "qt${qt_package_name_lowercase}")
        if (is_addon)
          list(
            APPEND installer_component_list
            "qt.qt${qt_major_version}.${qt_version_number_dotless}.addons.${addon}"
          )
          set(install_addon TRUE)
          break()
        endif()
      endforeach()

      if (NOT install_addon)
        set(install_extension FALSE)
        foreach(extension IN LISTS __qt_extensions)
          string(REGEX MATCH "^${extension}$" is_extension "qt${qt_package_name_lowercase}")
          if (is_extension)
            list(
              APPEND installer_component_list
              "extensions.${extension}.${qt_version_number_dotless}.${component_platform}"
            )
            set(install_extension TRUE)
            break()
          endif()
        endforeach()

        if (NOT install_extension)
          set(install_standalone_addon FALSE)
          foreach(standalone_addon IN LISTS __standalone_addons)
            string(REGEX MATCH "^${standalone_addon}$" is_standalone_addon "qt${qt_package_name_lowercase}")
            if (is_standalone_addon)
              list(
                APPEND installer_component_list
                "qt.qt${qt_major_version}.${qt_version_number_dotless}.${standalone_addon}"
              )
              set(install_standalone_addon TRUE)
              break()
            endif()
          endforeach()

          if(NOT install_standalone_addon)
            # Install the Desktop package
            list(
              APPEND installer_component_list
              "qt.qt${qt_major_version}.${qt_version_number_dotless}.${component_platform}"
            )
          endif()
        endif()
      endif()
    endforeach()

    qt_maintenance_tool_remove_installed_components(installer_component_list)
    list(LENGTH installer_component_list installer_component_list_size)
    if (installer_component_list_size EQUAL 0)
      return()
    endif()

    list(TRANSFORM qt_package_list PREPEND "Qt${qt_major_version}")
    list(JOIN qt_package_list " " qt_packages_as_string)
    list(JOIN installer_component_list " " installer_components_as_string)

    message(STATUS "Qt Creator: CMake could not find: ${qt_packages_as_string}. "
                   "Now installing: ${installer_components_as_string} "
                   "with the MaintenanceTool ...")

    if (QT_CREATOR_MAINTENANCE_TOOL_PROVIDER_USE_CLI)
      message(STATUS "Qt Creator: Using MaintenanceTool in CLI Mode. "
                     "Set QT_CREATOR_MAINTENANCE_TOOL_PROVIDER_USE_CLI to OFF for GUI mode.")
      execute_process(
        COMMAND
          "${QT_MAINTENANCE_TOOL}"
          --accept-licenses
          --default-answer
          --confirm-command
          install ${installer_component_list}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
        TIMEOUT 600
      )
      if (NOT result EQUAL 0)
        message(WARNING "Qt MaintenanceTool returned an error.\n${output}")
      endif()
    else()
      message(STATUS "Qt Creator: Using MaintenanceTool in GUI Mode. "
                     "Set QT_CREATOR_MAINTENANCE_TOOL_PROVIDER_USE_CLI to ON for CLI mode.")
      set(ENV{QTC_MAINTENANCE_TOOL_COMPONENTS} "${installer_component_list}")
      set(ENV{QTC_MAINTENANCE_TOOL_QT_PACKAGES} "${qt_package_list}")

      if(CMAKE_HOST_WIN32)
        # It's necessary to call actual test inside 'cmd.exe', because 'execute_process' uses
        # SW_HIDE to avoid showing a console window, it affects other GUI as well.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/17690 for details.
        #
        # Run the command using the proxy 'call' command to avoid issues related to invalid
        # processing of quotes and spaces in cmd.exe arguments.
        set(extra_runner cmd /d /c call)
      endif()

      execute_process(
        COMMAND ${extra_runner}
          "${QT_MAINTENANCE_TOOL}"
          --script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/maintenance_tool_provider.qs"
          --verbose
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
        ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
        TIMEOUT 600
      )
      if (NOT result EQUAL 0)
        message(WARNING "Qt MaintenanceTool returned an error.\n${output}")
      endif()

    endif()
  endif()
endfunction()

macro(qt_maintenance_tool_dependency method package_name)
  # Work around upstream cmake issue: https://gitlab.kitware.com/cmake/cmake/-/issues/27169
  if(ANDROID
      AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.29
      AND NOT ANDROID_USE_LEGACY_TOOLCHAIN_FILE
      AND NOT CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS
      AND NOT QT_NO_SET_MAINTENANCE_TOOL_PROVIDER_POLICY_CMP0155
    )
    message(DEBUG
      "Setting policy CMP0155 to OLD inside qt_maintenance_tool_dependency to avoid "
      "issues with not being able to find the Threads package when targeting Android with "
      "cmake_minimum_required(3.29) and CMAKE_CXX_STANDARD >= 20."
    )
    # We don't create an explicit policy stack entry before changing the policy, because setting
    # the policy value bubbles it up to the parent find_package() scope, which is fine, that's
    # exactly the scope we want to modify.
    cmake_policy(SET CMP0155 OLD)
  endif()

  if (${package_name} MATCHES "^Qt([5-9])(.*)$")
    set(__qt_dependency_qt_major_version ${CMAKE_MATCH_1})
    set(__qt_dependency_qt_package_name ${CMAKE_MATCH_2})

    # https://cmake.org/cmake/help/latest/command/find_package.html
    set(__qt_dependency_options
      CONFIG NO_MODULE MODULE REQUIRED EXACT QUIET GLOBAL NO_POLICY_SCOPE NO_DEFAULT_PATH NO_PACKAGE_ROOT_PATH
      NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_PACKAGE_REGISTRY
      NO_CMAKE_SYSTEM_PATH NO_CMAKE_INSTALL_PREFIX NO_CMAKE_SYSTEM_PACKAGE_REGISTRY CMAKE_FIND_ROOT_PATH_BOTH
      ONLY_CMAKE_FIND_ROOT_PATH NO_CMAKE_FIND_ROOT_PATH
    )
    set(__qt_dependency_oneValueArgs REGISTRY_VIEW)
    set(__qt_dependency_multiValueArgs COMPONENTS OPTIONAL_COMPONENTS NAMES CONFIGS HINTS PATHS PATH_SUFFIXES)
    cmake_parse_arguments(__qt_dependency_arg "${__qt_dependency_options}" "${__qt_dependency_oneValueArgs}" "${__qt_dependency_multiValueArgs}" ${ARGN})

    if (__qt_dependency_arg_REQUIRED AND __qt_dependency_arg_COMPONENTS)
      # Install missing COMPONENTS.
      set(__qt_dependency_pkgs_to_install "")
      foreach(pkg IN LISTS __qt_dependency_arg_COMPONENTS)
        find_package(Qt${__qt_dependency_qt_major_version}${pkg}
          PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_MODULE_PATH} NO_DEFAULT_PATH BYPASS_PROVIDER QUIET)
        if (NOT Qt${__qt_dependency_qt_major_version}${pkg}_FOUND)
          list(APPEND __qt_dependency_pkgs_to_install ${pkg})
        endif()
      endforeach()
      if (__qt_dependency_pkgs_to_install)
        qt_maintenance_tool_install("${__qt_dependency_qt_major_version}" "${__qt_dependency_pkgs_to_install}")
      endif()
    elseif(__qt_dependency_arg_REQUIRED AND NOT __qt_dependency_qt_package_name)
      # Install the Desktop package if Qt::Core is missing
      find_package(Qt${__qt_dependency_qt_major_version}Core
        PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_MODULE_PATH} NO_DEFAULT_PATH BYPASS_PROVIDER QUIET)
      if (NOT Qt${__qt_dependency_qt_major_version}$Core_FOUND)
        qt_maintenance_tool_install("${__qt_dependency_qt_major_version}" Core)
      endif()
    endif()

    find_package(${package_name} ${ARGN}
      PATHS ${CMAKE_PREFIX_PATH} ${CMAKE_MODULE_PATH} NO_DEFAULT_PATH BYPASS_PROVIDER QUIET)
    if (NOT ${package_name}_FOUND AND __qt_dependency_arg_REQUIRED)
      qt_maintenance_tool_install("${__qt_dependency_qt_major_version}" "${__qt_dependency_qt_package_name}")
      find_package(${package_name} ${ARGN} BYPASS_PROVIDER)
    endif()
  else()
    find_package(${package_name} ${ARGN} BYPASS_PROVIDER)
  endif()
endmacro()

cmake_language(
    SET_DEPENDENCY_PROVIDER qt_maintenance_tool_dependency
    SUPPORTED_METHODS FIND_PACKAGE
)
