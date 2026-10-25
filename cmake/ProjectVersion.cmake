# PROJECT_VER is the ESP-IDF application version, including prerelease suffixes.
# CI passes the precomputed release version with -DPROJECT_VER=... before building.
if(NOT DEFINED PROJECT_VER)
    find_package(Git QUIET)
    set(PROJECT_VER "0.0.0-dev+g0000000")
    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
            OUTPUT_VARIABLE template_revision OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(template_revision MATCHES "^[0-9a-f]+$")
            string(SUBSTRING "${template_revision}" 0 7 template_revision)
            set(PROJECT_VER "0.0.0-dev+g${template_revision}")
            execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags --exact-match --match "v[0-9]*"
                WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
                OUTPUT_VARIABLE template_tag OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            execute_process(COMMAND "${GIT_EXECUTABLE}" diff --quiet HEAD --
                WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." RESULT_VARIABLE template_dirty)
            if(template_tag MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+(-dev\\.[0-9]+)?$" AND template_dirty EQUAL 0)
                string(SUBSTRING "${template_tag}" 1 -1 PROJECT_VER)
            elseif(NOT template_dirty EQUAL 0)
                string(APPEND PROJECT_VER ".dirty")
            endif()
        endif()
    endif()
endif()
string(LENGTH "${PROJECT_VER}" template_version_length)
set(template_release_pattern "(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(-dev\\.[1-9][0-9]*)?")
set(template_snapshot_pattern "0\\.0\\.0-dev\\+g[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f](\\.dirty)?")
if(template_version_length GREATER 31 OR NOT PROJECT_VER MATCHES "^(${template_release_pattern}|${template_snapshot_pattern})$")
    message(FATAL_ERROR "PROJECT_VER must be a supported SemVer string of at most 31 ASCII bytes")
endif()
