add_contract(amend amend ${CMAKE_CURRENT_SOURCE_DIR}/src/amend.cpp)

target_include_directories(amend
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/telos-decide/contracts/decide/include)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

set_target_properties(amend
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/resources/amend.contracts.md ${CMAKE_CURRENT_BINARY_DIR}/resources/amend.contracts.md @ONLY )

target_compile_options( amend PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/resources -R${CMAKE_CURRENT_BINARY_DIR}/resources )
