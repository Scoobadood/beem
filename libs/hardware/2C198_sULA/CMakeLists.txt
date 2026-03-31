add_library(2C198_sULA SHARED
        src/2c198_sula.cc include/2c198_sula.h
)

target_include_directories(2C198_sULA
        PUBLIC  include)

target_link_libraries(2C198_sULA
        PUBLIC
        Core
        Acia
)