# Link the pthread library on Unix-like systems
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Add library
add_library(mylib SHARED)
target_sources(mylib PRIVATE library.cpp)
target_link_libraries(mylib PRIVATE Threads::Threads)
target_compile_options(mylib PRIVATE -finstrument-functions -rdynamic -fpic -fvisibility=default)# -finstrument-functions-exclude-file-list=/usr/lib,/usr/include,${CMAKE_SOURCE_DIR}/src/tracer.cpp,${CMAKE_SOURCE_DIR}/include/tracer.hpp,instrumentation.cpp,threaded-example.cpp,tracer.hpp)





# Add the executable
add_executable(threaded-example)

target_sources(
    threaded-example
    PRIVATE 
    threaded-example.cpp
    instrumentation.cpp 
    ${CMAKE_SOURCE_DIR}/src/tracer.cpp
)

target_link_libraries(threaded-example PRIVATE Threads::Threads mylib)
target_include_directories(threaded-example PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(threaded-example PRIVATE -rdynamic -finstrument-functions -finstrument-functions-exclude-file-list=/usr/lib,/usr/include,/usr/include/c++,${CMAKE_SOURCE_DIR}/src/tracer.cpp,${CMAKE_SOURCE_DIR}/include/tracer.hpp,instrumentation.cpp,threaded-example.cpp)




# target_link_options(mylib PRIVATE "-Wl,-export-dynamic")
# target_link_options(threaded-example PRIVATE "-Wl,-export-dynamic")