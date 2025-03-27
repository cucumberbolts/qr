# Test data encodation
add_executable(encoding_test encoding_test.c)
add_executable(error_test error_test.c)
add_executable(module_test module_test.c)

set(TESTS encoding_test error_test module_test)

# For IDEs
set_target_properties(${TESTS} PROPERTIES FOLDER "QR/Tests")

foreach(test IN LISTS TESTS)
    target_include_directories(${test} PRIVATE
        "${CMAKE_SOURCE_DIR}/include/"
        "${CMAKE_SOURCE_DIR}/src/"
        "${CMAKE_SOURCE_DIR}/lib/"
    )
endforeach()
