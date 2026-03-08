cmake_minimum_required(VERSION 3.0)
project(OpenRAVERawXml)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

find_package(OpenRAVE REQUIRED)
find_package(LibXml2 REQUIRED)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import distutils.sysconfig as sysconf; import os; \
    print(os.path.relpath(sysconf.get_python_lib(1, prefix='${CMAKE_INSTALL_PREFIX}'), '${CMAKE_INSTALL_PREFIX}'))"
    OUTPUT_VARIABLE PYTHON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
find_file(OPENRAVEPY_INT_SO_FILE openravepy_int.so PATHS ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_DIR}/openravepy/_openravepy_)

set(METADATA "${CMAKE_CURRENT_BINARY_DIR}/METADATA")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/dist-info/METADATA.in" "${METADATA}" IMMEDIATE @ONLY)

add_library(openrave_rawxml SHARED openrave_rawxml.cpp parsexml.cpp)

if(OpenRAVE_USE_PYBIND11_PYTHON3_BINDINGS)
  find_package(pybind11 2.9.2 REQUIRED)
  target_link_libraries(openrave_rawxml PUBLIC pybind11::module)
else()
  find_package(Boost REQUIRED COMPONENTS python thread)
  target_link_libraries(openrave_rawxml PUBLIC Boost::python Boost::thread)
endif()

target_link_libraries(openrave_rawxml PUBLIC Python3::Python LibXml2::LibXml2 ${OPENRAVEPY_INT_SO_FILE} ${OpenRAVE_LIBRARIES})
target_include_directories(openrave_rawxml PRIVATE ${OpenRAVE_INCLUDE_DIRS} ${OpenRAVE_PYTHON3_INCLUDE_DIRS})
set_target_properties(openrave_rawxml PROPERTIES PREFIX "" INSTALL_RPATH "$ORIGIN/openravepy/_openravepy_")
install(TARGETS openrave_rawxml DESTINATION ${PYTHON_INSTALL_DIR})
install(FILES ${METADATA} "${CMAKE_CURRENT_SOURCE_DIR}/dist-info/top_level.txt" DESTINATION ${PYTHON_INSTALL_DIR}/openrave_rawxml.dist-info/)
