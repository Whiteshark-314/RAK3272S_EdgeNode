# Future Tasks

- [x] Flash the current firmware to the Pico W and confirm live in ChirpStack that a dummy uplink lands every 5 minutes with an incrementing counter value. Done 2026-08-06: first uplink confirmed end-to-end (fPort 2, data `00`, fCnt 1, CRC_OK, RSSI -14/SNR 8.8). Still worth watching a couple more 5-minute cycles to confirm the counter keeps incrementing and nothing stalls.
- [ ] Fix the device profile's payload codec: it's decoding the raw dummy byte as `{"temp": 22.5}`, which is a leftover/misconfigured codec from earlier testing, not real data. Either disable the codec or update it so it doesn't imply real sensor readings while still sending dummy bytes.
- [ ] Device profile is set to `CLASS_C` but the firmware configures the module as Class A (`AT+CLASS=A`) — mismatch worth reconciling before relying on downlinks.
- [ ] Replace the dummy counter payload with real sensor data once a sensor is wired to the Pico W.
- [ ] Decide on a permanent data pipeline / storage & visualization (InfluxDB/Grafana was discussed and declined earlier in favor of the throwaway logger) — once decided, tear down the temporary `chirpstack-uplink-logger.service` on the gateway.
- [ ] Revisit confirmed vs. unconfirmed uplinks (`AT+CFM`) and duty-cycle/interval tuning once past first-test dummy data.
- [ ] Optional cleanup: the old sketchbook copy at `~/Arduino/RAK3272S_EdgeNode/` on the gateway no longer compiles (leftover duplicate `setup()`/`loop()` across two .ino files) and is superseded by this repo. Left alone per explicit request as of 2026-08-06 — revisit if it causes confusion later.
