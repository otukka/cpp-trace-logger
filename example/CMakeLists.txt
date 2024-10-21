# Link the pthread library on Unix-like systems
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Add library
add_library(mylib SHARED)
target_sources(mylib PRIVATE library.cpp)
target_link_libraries(mylib PRIVATE Threads::Threads)


# Add the executable
add_executable(main)

target_sources(
    main
    PRIVATE 
    main.cpp
    instrumentation.cpp
)

target_link_libraries(main PRIVATE mylib)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(main PRIVATE -rdynamic -finstrument-functions -finstrument-functions-exclude-file-list=/usr/lib,/usr/include,/usr/include/c++,${CMAKE_SOURCE_DIR}/include/tracer.hpp,instrumentation.cpp)




# target_link_options(mylib PRIVATE "-Wl,-export-dynamic")
target_link_options(main PRIVATE "-Wl,-export-dynamic")