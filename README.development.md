## Development

### OTA

### Generating certificates for HTTPS

```
openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 365 -nodes
```

### Running HTTPS server

```
cd src
openssl s_server -WWW -key main/certs/ca_key_ota.pem -cert main/certs/ca_cert_ota.pem -port 8070
```

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
