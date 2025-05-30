set(MODULE_NAME "TestGdi")
set(MODULE_PREFIX "TEST_GDI")

disable_warnings_for_directory(${CMAKE_CURRENT_BINARY_DIR})

set(${MODULE_PREFIX}_DRIVER ${MODULE_NAME}.c)

set(${MODULE_PREFIX}_TESTS
    TestGdiRop3.c
    #	TestGdiLine.c # TODO: This test is broken
    TestGdiRegion.c
    TestGdiRect.c
    TestGdiBitBlt.c
    TestGdiCreate.c
    TestGdiEllipse.c
    TestGdiClip.c
)

create_test_sourcelist(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_DRIVER} ${${MODULE_PREFIX}_TESTS})

include_directories(..)

add_library(helpers STATIC helpers.c)
target_link_libraries(helpers freerdp)

add_executable(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

target_link_libraries(${MODULE_NAME} winpr freerdp helpers)

set_target_properties(${MODULE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TESTING_OUTPUT_DIRECTORY}")

foreach(test ${${MODULE_PREFIX}_TESTS})
  get_filename_component(TestName ${test} NAME_WE)
  add_test(${TestName} ${TESTING_OUTPUT_DIRECTORY}/${MODULE_NAME} ${TestName})
endforeach()

set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER "FreeRDP/Test")
