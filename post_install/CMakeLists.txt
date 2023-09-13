install(CODE "execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/test)")
install(CODE "execute_process(COMMAND cmake ${CMAKE_SOURCE_DIR}/test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)")
