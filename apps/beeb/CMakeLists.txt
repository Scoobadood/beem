add_executable(beeb
        src/main.cc
        src/beeb_worker.cc include/beeb_worker.h

        src/crt_window/crt_window.cc include/crt_window.h
        src/crt_window/vdu_view.cc include/vdu_view.h
        src/crt_window/key_mapper.cc include/key_mapper.h
        src/crt_window/led_label_widget.cc include/led_label_widget.h

        src/disassembly_view/debugger_window.cc include/debugger_window.h
        src/disassembly_view/disassembly_view.cc include/disassembly_view.h
        src/disassembly_view/register_view.cc include/register_view.h

        src/memory_view/memory_window.cc include/memory_window.h
        src/memory_view/memory_view.cc include/memory_view.h

        src/cassette_window/cassette_window.cc include/cassette_window.h

        src/window_mediator.cc include/window_mediator.h

        include/breakpoint_manager.h
        src/disassembly_view/breakpoint_dialog.cc include/breakpoint_dialog.h

        src/disassembly_view/bus_view.cc include/bus_view.h
#       src/string_to_keystrokes.cpp include/string_to_keystrokes.h
)

target_include_directories(beeb
        PUBLIC include)

target_link_libraries(beeb
        PRIVATE
        Beeb
        Disassembler
        UEF
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)