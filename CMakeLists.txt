cmake_minimum_required(VERSION 3.12)
project(exhaust_pi VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Verzeichnis mit ocpn_plugin.h (aus den OpenCPN-Quellen, Ordner include/)
set(OCPN_API_DIR "" CACHE PATH "Verzeichnis mit ocpn_plugin.h")
if(NOT EXISTS "${OCPN_API_DIR}/ocpn_plugin.h")
  message(FATAL_ERROR "ocpn_plugin.h nicht gefunden. Bitte -DOCPN_API_DIR=/pfad/zu/OpenCPN/include angeben.")
endif()

find_package(wxWidgets REQUIRED COMPONENTS aui adv core base)
include(${wxWidgets_USE_FILE})

add_library(exhaust_pi MODULE
  src/exhaust_pi.cpp
  src/exhaust_logic.cpp
  src/ExhaustWindow.cpp
  src/PrefsDialog.cpp
)

target_include_directories(exhaust_pi PRIVATE src "${OCPN_API_DIR}")
target_link_libraries(exhaust_pi PRIVATE ${wxWidgets_LIBRARIES})

if(MSVC)
  target_compile_options(exhaust_pi PRIVATE /utf-8)
  # Unter Windows muss zusätzlich gegen opencpn.lib gelinkt werden
  set(OCPN_LIB "" CACHE FILEPATH "Pfad zu opencpn.lib (nur Windows)")
  target_link_libraries(exhaust_pi PRIVATE "${OCPN_LIB}")
endif()

install(TARGETS exhaust_pi LIBRARY DESTINATION lib/opencpn)

# Szenario-Tests der Alarmlogik (ohne OpenCPN/wxWidgets): make test_logic && ./test_logic
add_executable(test_logic EXCLUDE_FROM_ALL tests/test_logic.cpp src/exhaust_logic.cpp)
target_include_directories(test_logic PRIVATE src)
