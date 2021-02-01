# find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories (${TEST_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
        ${AALWINES_SOURCE_DIR}
        )
# add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (GenerateQueries QueryTemplate.cpp GenerateQueries.cpp )

target_link_libraries(GenerateQueries ${Boost_LIBRARIES} aalwines)
