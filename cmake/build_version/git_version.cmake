# Get the branch name from Git
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${PROJ_ROOT}
    OUTPUT_VARIABLE APP_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Get the version from Git
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${PROJ_ROOT}
    OUTPUT_VARIABLE APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Fallbacks for CI/CD environments
if(APP_VERSION STREQUAL "")
    if(DEFINED ENV{GITHUB_REF_NAME} AND NOT "$ENV{GITHUB_REF_NAME}" STREQUAL "")
        set(APP_VERSION "$ENV{GITHUB_REF_NAME}")
    elseif(DEFINED ENV{GITHUB_SHA})
        string(SUBSTRING "$ENV{GITHUB_SHA}" 0 7 APP_VERSION)
    else()
        set(APP_VERSION "unknown")
    endif()
endif()

# Combine branch and version
if(NOT APP_BRANCH STREQUAL "" AND NOT APP_BRANCH STREQUAL "HEAD")
    set(APP_VERSION "${APP_BRANCH}-${APP_VERSION}")
endif()

# Generate the header using the specific paths passed in from CMakeLists.txt
configure_file(
    ${TEMPLATE_FILE}
    ${OUTPUT_FILE}
    @ONLY
)