function(find_or_download_data_tamer)

    if (NOT TARGET data_tamer_parser)
        message(STATUS "data_tamer not found, downloading")
        CPMAddPackage(
            NAME data_tamer
            URL https://github.com/PickNikRobotics/data_tamer/archive/refs/tags/1.0.3.zip
            DOWNLOAD_ONLY YES
        )

        add_library(data_tamer_parser INTERFACE )
        target_include_directories(data_tamer_parser INTERFACE "${data_tamer_SOURCE_DIR}/data_tamer_cpp/include")
        add_library(data_tamer::parser ALIAS data_tamer_parser)

    endif()

endfunction()
