project(dtmf-cpp)

set(CMAKE_CXX_STANDARD 11)

add_library(dtmf-cpp
    DtmfDetector.hpp DtmfDetector.cpp
    DtmfGenerator.hpp DtmfGenerator.cpp
)

add_executable(detect-au detect-au.cpp)
target_link_libraries(detect-au dtmf-cpp)
