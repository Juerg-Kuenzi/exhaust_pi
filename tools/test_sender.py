#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Juerg Kuenzi
"""Sendet simulierte Abgastemperaturen als NMEA-XDR-Sätze per UDP an OpenCPN.

Szenarien (--scenario):
  sinus      Temperatur schwankt zwischen --min und --max (Standard 30..70 °C)
  impeller   Kaltstart, Aufwärmen auf 42 °C, nach --fail-after s Totalausfall
             des Impellers (+20 °C/min) -> Anstiegsalarm vor dem Alarmwert
  teildefekt Kaltstart, Aufwärmen, dann langsamer Anstieg (+1 °C/min) bis 54 °C
             -> Vorwarnung, aber kein Alarm

Beispiel:
    python3 test_sender.py --scenario impeller --fail-after 180
Mit Strg+C abbrechen -> nach dem Timeout meldet das Plugin "Keine Daten".
"""
import argparse
import math
import random
import socket
import time


def nmea(body: str) -> str:
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return f"${body}*{cs:02X}\r\n"


def warm_up(t, start=18.0, end=42.0, tau=90.0):
    return end - (end - start) * math.exp(-t / tau)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=1456)
    p.add_argument("--name", default="EXHAUST")
    p.add_argument("--scenario", choices=["sinus", "impeller", "teildefekt"], default="sinus")
    p.add_argument("--min", type=float, default=30.0)
    p.add_argument("--max", type=float, default=70.0)
    p.add_argument("--period", type=float, default=60.0, help="Sekunden pro Sinuszyklus")
    p.add_argument("--fail-after", type=float, default=300.0,
                   help="Sekunden bis zum Impellerdefekt (Szenarien impeller/teildefekt)")
    a = p.parse_args()

    def profile(t):
        if a.scenario == "sinus":
            mid, amp = (a.max + a.min) / 2, (a.max - a.min) / 2
            return mid + amp * math.sin(2 * math.pi * t / a.period)
        base = warm_up(t)
        dt = max(0.0, t - a.fail_after)
        if a.scenario == "impeller":
            return base + dt * 20.0 / 60.0
        return min(54.0, base + dt / 60.0)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    t0 = time.time()
    while True:
        t = time.time() - t0
        temp = profile(t) + random.gauss(0, 0.15)
        # Gleicher Aufbau wie das Yacht-Devices-Gateway: Luftdruck + Abgastemperatur
        msg = nmea(f"YDXDR,P,101320,P,Baro,C,{temp:.1f},C,{a.name}")
        sock.sendto(msg.encode("ascii"), (a.host, a.port))
        print(f"{t:6.0f} s  {msg.strip()}")
        time.sleep(1)


if __name__ == "__main__":
    main()
