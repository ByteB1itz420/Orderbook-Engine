add_executable(lob_tests
    test_types.cpp
    test_price_level.cpp
    test_order_pool.cpp
    test_events.cpp
    test_order_book_api.cpp
)
target_link_libraries(lob_tests PRIVATE lob_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(lob_tests)
