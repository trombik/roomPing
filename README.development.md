## Development

### Environment variable

#### `IDF_PATH`

`IDF_PATH` must be path to `esp-idf` directory.

```
export IDF_PATH=~/github/trombik/esp-idf
```

#### `PATH`

`PATH` must include path to the tool-chain `bin` directory, where
`xtensa-esp32-elf-gcc` and others are.

```
export PATH="/usr/local/xtensa-esp32-elf/bin:$PATH"
```

#### `ESPPORT`

Set `ESPPORT` to serial port of your device.

```
export ESPPORT=/dev/ttyU0
```
