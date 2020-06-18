## Development

### Required applications

* `git`
* `openssl`
* an HTTP client, such as `fetch(1)`, `wget`, or `curl`
* `esp-idf` and its tool-chains installed
* `astyle` (optional)

### Environment variables

#### `IDF_PATH`

`IDF_PATH` must be path to `esp-idf` directory.

```console
export IDF_PATH=~/github/trombik/esp-idf
```

#### `PATH`

`PATH` must include path to the tool-chain `bin` directory, where
`xtensa-esp32-elf-gcc` and others are.

```console
export PATH="/usr/local/xtensa-esp32-elf/bin:$PATH"
```

#### `ESPPORT`

Set `ESPPORT` to serial port of your device.

```console
export ESPPORT=/dev/ttyU0
```

#### Generating self-signed certificates for HTTPS

The `OTA` process fetches firmware over HTTPS. Create your own self-signed
certificates.

```console
> pwd
/home/trombik/github/trombik/roomPing
> openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 365 -nodes
Generating a RSA private key
...........+++++
...........................................................................+++++
writing new private key to 'ca_key.pem'
-----
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:
State or Province Name (full name) [Some-State]:
Locality Name (eg, city) []:
Organization Name (eg, company) [Internet Widgits Pty Ltd]:
Organizational Unit Name (eg, section) []:
Common Name (e.g. server FQDN or YOUR name) []:192.168.1.100
Email Address []:
```

Note that, when `openssl` asks questions, you just hit enter key except when
it asks `Common Name`. The `Common Name` should be the IP address or a valid
DNS name of HTTPS server. Make sure this matches the IP address or the host
name of `PROJECT_LATEST_APP_URL`.

Copy the keys.

```console
> cp ca_key.pem  src/main/certs/ca_key_ota.pem
> cp ca_cert.pem src/main/certs/ca_cert_ota.pem
```

#### Set `PROJECT_LATEST_APP_URL`

Navigate to `Project configuration` -> `URL to the latest application`. Set
the value to: `https://ip.add.re.ss:8070/build/roomPing.bin`.

Make sure to replace `ip.add.re.ss` with the actual IP address of the local
machine.

```console
> $IDF_PATH/tools/idf.py menuconfig
```

#### Running HTTPS server on the local machine

Run HTTPS server. Do NOT use `openssl s_server -HTTP`. You should use a
full-fledged HTTPS server, such as `nginx`, to avoid weird issues.  An example
`nginx.conf` is located at [`tools/nginx.conf`](tools/nginx.conf).

Make sure the URL works.

`wget(1)` example output:

```console
> pwd
/home/trombik/github/trombik/roomPing/src
> wget --ca-certificate=main/certs/ca_cert_ota.pem https://192.168.1.54:8070/build/roomPing.bin
--2019-12-02 13:43:11--  https://192.168.1.54:8070/build/roomPing.bin
Connecting to 192.168.1.54:8070... connected.
HTTP request sent, awaiting response... 200 ok
Length: unspecified [text/plain]
Saving to: ‘roomPing.bin’
```

`fetch(1)` example output:

```console
> pwd
/home/trombik/github/trombik/roomPing/src
> fetch --ca-cert main/certs/ca_cert_ota.pem  https://192.168.1.54:8070/build/roomPing.bin
fetch: https://192.168.1.54:8070/build/roomPing.bin: size of remote file is not known
roomPing.bin                                           826 kB  443 MBps    00s
```

#### Building the latest firmware and flashing it

To perform `OTA`, the firmware on the device must be newer than `FIXME`. If
not, update the firmware over USB serial.

```console
> pwd
/home/trombik/github/trombik/roomPing
> cd src
> $IDF_PATH/tools/idf.py flash
```

#### Performing `OTA`

Make sure the version of the running firmware is older than the new firmware.

Set run to `esp/ota/set`.

```console
> mosquitto_pub -h ip.add.re.ss -t 'homie/foo/esp/reboot/set' -m 'reboot'
```
