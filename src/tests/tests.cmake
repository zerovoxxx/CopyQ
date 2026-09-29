message(STATUS "Building with tests.")

file(GLOB copyq_tests_SOURCES tests/*.cpp)
list(REMOVE_ITEM copyq_tests_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests_palette.cpp")
list(REMOVE_ITEM copyq_tests_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests_management.cpp")
if(APPLE)
    set_source_files_properties(tests/clipboardguard.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()
add_executable(copyq-tests
    ${copyq_tests_SOURCES}
    $<TARGET_OBJECTS:copyq-common>
    ${MINIAUDIO_OBJECTS}
)
set_target_properties(copyq-tests PROPERTIES COMPILE_DEFINITIONS "${copyq_DEFINITIONS}")

find_package(${copyq_qt}Test REQUIRED)
target_link_libraries(copyq-tests ${copyq_LIBRARIES} ${copyq_qt}::Test)
target_include_directories(copyq-tests PRIVATE .)

add_executable(copyq-palette-tests
    tests/tests_palette.cpp
    tests/clipboardguard.cpp
    $<TARGET_OBJECTS:copyq-common>
    ${copyq_RESOURCES_RCC}
    ${MINIAUDIO_OBJECTS}
)
target_compile_definitions(copyq-palette-tests PRIVATE ${copyq_DEFINITIONS})
target_link_libraries(copyq-palette-tests PRIVATE ${copyq_LIBRARIES} ${copyq_qt}::Test)
target_include_directories(copyq-palette-tests PRIVATE .)

add_executable(copyq-management-tests
    tests/tests_management.cpp
    tests/clipboardguard.cpp
    $<TARGET_OBJECTS:copyq-common>
    ${copyq_RESOURCES_RCC}
    ${MINIAUDIO_OBJECTS}
)
target_compile_definitions(copyq-management-tests PRIVATE ${copyq_DEFINITIONS})
target_link_libraries(copyq-management-tests PRIVATE ${copyq_LIBRARIES} ${copyq_qt}::Test)
target_include_directories(copyq-management-tests PRIVATE .)

set(copyq_pkg itemtests)
set(copyq_plugin_SOURCES
    tests/itemtests/itemtests.cpp
    item/itemwidget.cpp
    )
add_library(${copyq_pkg} MODULE ${copyq_plugin_SOURCES})
target_link_libraries(${copyq_pkg} ${copyq_qt}::Widgets ${copyq_qt}::Test)
target_include_directories(${copyq_pkg} PRIVATE .)
