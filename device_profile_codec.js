/**
 * Decode uplink function
 *
 * Payload layout (5 bytes, big-endian), sent by RAK3272S_EdgeNode.ino:
 *   [0..1] int16  mcu_temp_c   * 100  -- REAL: RP2040 internal core (die) temperature.
 *                                        Not an ambient/room reading.
 *   [2..3] int16  dummy_humidity * 100 -- DUMMY: simulated, not a real sensor.
 *   [4]    uint8  dummy_counter        -- DUMMY: increments every uplink, wraps at 256.
 *
 * @param {object} input
 * @param {number[]} input.bytes Byte array containing the uplink payload, e.g. [255, 230, 255, 0]
 * @param {number} input.fPort Uplink fPort.
 * @param {Record<string, string>} input.variables Object containing the configured device variables.
 *
 * @returns {{data: object, errors: string[], warnings: string[]}}
 * An object containing:
 * - data: Object representing the decoded payload.
 * - errors: An array of errors (optional).
 * - warnings: An array of warnings (optional).
 */
function decodeUplink(input) {
  var bytes = input.bytes;

  if (bytes.length !== 5) {
    return {
      data: {},
      errors: ["expected 5 bytes, got " + bytes.length],
    };
  }

  function toInt16(hi, lo) {
    var v = (hi << 8) | lo;
    if (v & 0x8000) {
      v -= 0x10000;
    }
    return v;
  }

  return {
    data: {
      mcu_temp_c: toInt16(bytes[0], bytes[1]) / 100,
      dummy_humidity_pct: toInt16(bytes[2], bytes[3]) / 100,
      dummy_counter: bytes[4],
    },
  };
}

/**
 * Encode downlink function.
 *
 * @param {object} input
 * @param {object} input.data Object representing the payload that must be encoded.
 * @param {Record<string, string>} input.variables Object containing the configured device variables.
 *
 * @returns {{bytes: number[], fPort: number, errors: string[], warnings: string[]}}
 * An object containing:
 * - bytes: Byte array containing the downlink payload.
 * - fPort: The downlink LoRaWAN fPort.
 * - errors: An array of errors (optional).
 * - warnings: An array of warnings (optional).
 */
function encodeDownlink(input) {
  return {
    fPort: 10,
    bytes: [225, 230, 255, 0],
  };
}
