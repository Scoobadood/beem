add_library(Rom SHARED src/rom.cpp)

target_include_directories(Rom PUBLIC include)

target_link_libraries(Rom PUBLIC Bus)