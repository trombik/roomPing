# `rooomPing`

`ICMP` _sensor_ on `ESP32`. The device periodically sends and receives `ICMP`
packets to/from destinations. The result in `influx` line protocol format is
published to MQTT broker. This is a project based on `esp-idf` SDK.

## Tested builds

| `esp-idf` version | hardware |
|-------------------|----------|
| `esp-idf` (master branch) | `WROOM32` |

## Project configuration

`make menuconfig` has `Project configuration` menu to modify default
configuration.

## Example log output

```
```
