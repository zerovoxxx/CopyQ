# Product metadata and icon on Windows.
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" copyq_WINDOWS_VERSION_MATCH "${copyq_version}")
set(copyq_WINDOWS_VERSION "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
set(copyq_RC "${CMAKE_CURRENT_BINARY_DIR}/qclip.rc")
configure_file(qclip.rc.in "${copyq_RC}" @ONLY)

file(GLOB copyq_SOURCES ${copyq_SOURCES}
    platform/win/winplatforminput.cpp
    platform/win/winplatform.cpp
    platform/win/winplatformclipboard.cpp
    platform/win/winplatformwindow.cpp
    platform/win/winwindoweffects.cpp
    platform/dummy/dummyclipboard.cpp
    platform/platformcommon.cpp
    ../qxt/qxtglobalshortcut_win.cpp
    )

set(USE_QXT TRUE)

# Omit opening extra console window on Windows.
set(copyq_windows_no_console WIN32)
list(APPEND copyq_COMPILE
    ${copyq_COMPILE}
    ${copyq_RC}
    )

if (MSVC)
    set(copyq_LINK_FLAGS ${copyq_LINK_FLAGS} "/ENTRY:mainCRTStartup")
endif()

list(APPEND copyq_LIBRARIES imm32 dwmapi)
