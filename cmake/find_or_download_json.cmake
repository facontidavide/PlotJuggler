function(find_or_download_json)

    find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp)

    if(NLOHMANN_JSON_INCLUDE_DIR)
        message(STATUS "Found nlohmann_json in system")

        add_library(nlohmann_json INTERFACE IMPORTED)
        set_target_properties(nlohmann_json PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}"
        )
        set(NLOHMANN_JSON_FOUND TRUE)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    endif()

    if (NOT NLOHMANN_JSON_FOUND AND NOT TARGET nlohmann_json)
        message(STATUS "nlohmann_json not found, downloading")
        CPMAddPackage(
            NAME nlohmann_json
            URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip
            DOWNLOAD_ONLY YES
        )

        add_library(nlohmann_json INTERFACE )
        target_include_directories(nlohmann_json INTERFACE
            $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

        set(NLOHMANN_JSON_FOUND TRUE)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    endif()

endfunction()
