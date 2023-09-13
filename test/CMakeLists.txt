cmake_minimum_required(VERSION 3.5)

add_subdirectory(xtypes_generator_test_type EXCLUDE_FROM_ALL)

if(APPLE)
  set(LIB_SUFFIX dylib)
else(APPLE)
  set(LIB_SUFFIX so)
endif()

add_executable(XType_test EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests.cpp
)
add_dependencies(XType_test xtypes_generator_test_type_cpp)
target_compile_features(XType_test PUBLIC cxx_std_17) # Use C++17
target_include_directories(XType_test
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(XType_test
	PUBLIC
	nlohmann_json::nlohmann_json
	${XTYPES_CPP_TARGET}
	xtypes_generator_test_type_cpp
	"-Wl,--disable-new-dtags"
)

# from cmake3.13 target_link_options(XType_test PUBLIC "-Wl,--disable-new-dtags") # used so that we test the file in build not the installed lib
#if(APPLE)
#  target_link_libraries(XType_test ${PKGCONFIGtest_LIBRARIES} ${CMAKE_BINARY_DIR}/lib${XTYPES_CPP_TARGET}.dylib) # Link order matters xtype/.
#else(APPLE)
#  target_link_libraries(XType_test PUBLIC
#		nlohmann_json::nlohmann_json
#		${XTYPES_CPP_TARGET}
#		"-Wl,--disable-new-dtags"
#  )
#endif()

if(Catch_FOUND)
  catch_discover_tests(XType_test)
endif()

add_custom_target(cpp_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/XType_test
    DEPENDS XType_test
)





