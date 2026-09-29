add_executable(lob_tests
    test_types.cpp
    test_price_level.cpp
    test_order_pool.cpp
    test_events.cpp
    test_order_book_api.cpp
    test_matching.cpp
    test_lobster_replay.cpp
    test_contract.cpp
    test_compact.cpp
)
target_link_libraries(lob_tests PRIVATE lob_core Catch2::Catch2WithMain)
target_compile_definitions(lob_tests PRIVATE LOB_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

include(Catch)
catch_discover_tests(lob_tests)
