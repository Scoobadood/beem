add_library(5C095_vULA SHARED
        src/5c095_vula.cpp)

target_include_directories(5C095_vULA PUBLIC
        include)

target_link_libraries(5C095_vULA PUBLIC
        Bus)