# Future Tasks

- [ ] Flash the current firmware to the Pico W and confirm live in ChirpStack (Applications -> RAK3172S app -> First Edge Node -> Events) that a dummy uplink lands every 5 minutes with an incrementing counter value.
- [ ] Replace the dummy counter payload with real sensor data once a sensor is wired to the Pico W.
- [ ] Decide on a permanent data pipeline / storage & visualization (InfluxDB/Grafana was discussed and declined earlier in favor of the throwaway logger) — once decided, tear down the temporary `chirpstack-uplink-logger.service` on the gateway.
- [ ] Revisit confirmed vs. unconfirmed uplinks (`AT+CFM`) and duty-cycle/interval tuning once past first-test dummy data.
- [ ] Optional cleanup: the old sketchbook copy at `~/Arduino/RAK3272S_EdgeNode/` on the gateway no longer compiles (leftover duplicate `setup()`/`loop()` across two .ino files) and is superseded by this repo. Left alone per explicit request as of 2026-08-06 — revisit if it causes confusion later.
