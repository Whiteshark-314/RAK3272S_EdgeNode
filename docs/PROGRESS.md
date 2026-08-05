# Progress Log

## 2026-08-04
- ChirpStack application "RAK3172S" created (name is a leftover typo — the actual hardware is RAK3272S) with two devices: "First Edge Node" (OTAA, DevEUI `ac1f09fffe1141b6`) and "First Edge Node ABP" (DevEUI `ac1f09fffe1141b5`). Device profiles set up.
- Temporary uplink logger (`chirpstack-uplink-logger.service`) installed on the gateway (`rak-gateway`) to capture test uplinks to `/var/log/chirpstack-uplinks.jsonl` while the permanent data pipeline is still undecided.

## 2026-08-05 / 2026-08-06
- Wiring finalized: RAK3272S RST -> Pico GPIO2, module UART on Pico UART1/`Serial2` (GPIO4 TX, GPIO5 RX), power from Pico `3V3(OUT)`/GND.
- Dev environment set up directly on `rak-gateway` (it has a desktop, so this was feasible): `arduino-cli`, the `rp2040:rp2040` board core (arduino-pico by Earle Philhower, board `rp2040:rp2040:rpipicow`), VS Code + `vscode-arduino.vscode-arduino-community` extension (Arduino IDE 2.x has no official Linux ARM64 build).
- Wrote and confirmed working: AT-command based OTAA join on boot (region IN865/band 3, class A), with manual AT command passthrough over the Serial Monitor. **Join confirmed successful on real hardware.**
- Turned this into a real git repo at `~/RAK3272S_EdgeNode/` and extended the firmware to send one dummy uplink (incrementing counter byte, `AT+SEND=2:<hex>`) every 5 minutes after join, on top of the working join logic. Compiles clean for the Pico W target.
- Added `docs/RUI3_AT_Command_Manual.md` — an offline mirror of RAKwireless's RUI3 AT Command Manual, so command syntax (e.g. `AT+SEND=<port>:<payload>`) can be checked without needing network access.
