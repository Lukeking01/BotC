cmake_minimum_required(VERSION 3.16)
project(BotC_Storyteller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt6 AUTOMOC, AUTOUIC, AUTORCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 Widgets
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Source and header files
set(SOURCES
    main.cpp
    storyteller.cpp
    CharacterSelectionDialog.cpp
    BluffSelectionDialog.cpp
)

set(HEADERS
    storyteller.h
    CharacterSelectionDialog.h
    BluffSelectionDialog.h
)

# Create executable
add_executable(botc ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(botc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link Qt6 and nlohmann_json
target_link_libraries(botc
    PRIVATE
        Qt6::Widgets
        nlohmann_json::nlohmann_json
)

# Optional: install
install(TARGETS botc RUNTIME DESTINATION bin)
