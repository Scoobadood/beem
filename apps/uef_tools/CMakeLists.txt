add_executable(uef-tools
        src/main.cc
)

#target_include_directories(uef-tools
#)

target_link_libraries(uef-tools
        PRIVATE UEF
)