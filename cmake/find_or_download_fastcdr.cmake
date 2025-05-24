function(find_or_download_fastcdr)

    ##### fastCDR ######
    find_package(fastcdr QUIET)

    if (fastcdr_FOUND)
        message(STATUS "Found fastcdr in system")

    elseif(NOT TARGET fastcdr)
        message(STATUS "fastcdr not found, downloading")
        CPMAddPackage(
            NAME fastcdr_imported
            GITHUB_REPOSITORY eProsima/Fast-CDR
            GIT_TAG v2.2.6
            OPTIONS "BUILD_SHARED_LIBS OFF"
        )
    endif()

endfunction()
