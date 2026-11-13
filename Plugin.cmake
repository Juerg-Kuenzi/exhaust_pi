# ~~~
# Summary:      Projektspezifische Einstellungen für ExhaustTemp
# Copyright (C) 2026 Juerg Kuenzi
# License:      GPLv3+ (Aufbau übernommen aus der Vorlage shipdriver_pi)
# ~~~

# -------- Optionen ----------
# Ziel-Repositories für ein späteres Hochladen in den Plugin-Katalog
# (Cloudsmith). Ohne CLOUDSMITH_API_KEY wird nichts hochgeladen.
set(OCPN_TEST_REPO
    "juerg-kuenzi/exhaust-alpha"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "juerg-kuenzi/exhaust-beta"
    CACHE STRING "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "juerg-kuenzi/exhaust-prod"
    CACHE STRING "Default repository for tagged builds not matching 'beta'"
)

# -------  Plugin --------
set(PKG_NAME exhaust_pi)
set(PKG_VERSION 1.1.1)  # Major.Minor.Patch
set(PKG_PRERELEASE "")  # leer oder z. B. 'beta'

set(DISPLAY_NAME ExhaustTemp)     # Dialoge, Paketnamen
set(PLUGIN_API_NAME ExhaustTemp)  # muss GetCommonName() entsprechen
set(PKG_SUMMARY "Abgastemperatur mit Vorwarnung, Alarm und Anstiegsüberwachung")
set(PKG_DESCRIPTION [=[
Zeigt die Abgastemperatur eines nassen Auspuffs aus NMEA-0183-XDR-Sätzen an
und überwacht sie mit Vorwarnung, Alarmwert und Anstiegsalarm oberhalb der
minimalen Betriebstemperatur. Meldet optional auch einen Datenausfall.
]=])

set(PKG_AUTHOR "Juerg Kuenzi")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://github.com/Juerg-Kuenzi/exhaust_pi)
set(PKG_INFO_URL https://github.com/Juerg-Kuenzi/exhaust_pi)

set(SRC
    src/exhaust_pi.h
    src/exhaust_pi.cpp
    src/exhaust_logic.h
    src/exhaust_logic.cpp
    src/exhaust_types.h
    src/ExhaustWindow.h
    src/ExhaustWindow.cpp
    src/PrefsDialog.h
    src/PrefsDialog.cpp
)

set(PKG_API_LIB api-18)  # Verzeichnis in opencpn-libs/, OpenCPN ab 5.7

macro(late_init)
  # Quelltexte sind UTF-8 (Umlaute, °C). MSVC braucht dafür /utf-8.
  if (MSVC)
    target_compile_options(${PACKAGE_NAME} PRIVATE /utf-8)
  endif ()
  target_include_directories(${PACKAGE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
endmacro ()

macro(add_plugin_libraries)
  # Keine zusätzlichen Bibliotheken nötig
endmacro ()
