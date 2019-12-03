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

### `OTA` process

1. The device check if it should update the firmware by requesting
   `CONFIG_PROJECT_LATEST_APP_AVAILABLE`.
2. If the status code of the request is 200, fetch a firmware at
   `CONFIG_PROJECT_LATEST_APP_URL`. If not, sleep 60 sec.
3. If the version of the fetched firmware is newer than the one of the running
   firmware, start the `OTA` process. If not, sleep 60 sec.
4. Repeat.

`src/version.txt` contains version number.

#### Generating self-signed certificates for HTTPS

The `OTA` process fetches firmware over HTTPS. Create your own self-signed
certificates.

```
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

```
> cp ca_key.pem  src/main/certs/ca_key_ota.pem
> cp ca_cert.pem src/main/certs/ca_cert_ota.pem
```

#### Set `PROJECT_LATEST_APP_URL`

Navigate to `Project configuration` -> `URL to the latest application`. Set
the value to:

```
https://ip.add.re.ss:8070/build/roomPing.bin
```

Make sure to replace `ip.add.re.ss` with the actual IP address of the local
machine.

```
> $IDF_PATH/tools/idf.py menuconfig
```

#### Running HTTPS server on the local machine

Run HTTPS server. Do NOT use `openssl s_server -HTTP`. You should use a
full-fledged HTTPS server, such as `nginx`, to avoid weird issues.  An example
`nginx.conf` is located at [`tools/nginx.conf`](tools/nginx.conf).

Make sure the URL works.

`wget(1)` example output:

```
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

```
> pwd
/home/trombik/github/trombik/roomPing/src
> fetch --ca-cert main/certs/ca_cert_ota.pem  https://192.168.1.54:8070/build/roomPing.bin
fetch: https://192.168.1.54:8070/build/roomPing.bin: size of remote file is not known
roomPing.bin                                           826 kB  443 MBps    00s
```

#### Building the latest firmware and flashing it

To perform `OTA`, the firmware on the device must be newer than `FIXME`. If
not, update the firmware over USB serial.

```
> pwd
/home/trombik/github/trombik/roomPing
> cd src
> $IDF_PATH/tools/idf.py flash
```

#### Create a file to tell the device to update the firmware

```
> pwd
/home/trombik/github/trombik/roomPing/src
> touch update
```

#### Performing `OTA`

1. Increment version number in `src/version.txt`
2. Build the project as usual
3. Wait for the device to update the firmware

An example log when newer version is available:

```
...
I (5997) task_ota: Fetching the update
URL: https://192.168.1.54:8070/build/roomPing.bin
...
I (8727) task_ota: Writing to partition subtype 16 at offset 0x110000
I (8727) task_ota: New firmware version: 2
I (8727) task_ota: Running firmware version: 1
I (10277) task_ota: esp_ota_begin succeeded
...
I (51107) task_ota: Connection closed
I (51107) task_ota: Total Write binary data length: 846704
I (51107) esp_image: segment 0: paddr=0x00110020 vaddr=0x3f400020 size=0x22e58 (142936) map
I (51167) esp_image: segment 1: paddr=0x00132e80 vaddr=0x3ffb0000 size=0x03324 ( 13092)
I (51177) esp_image: segment 2: paddr=0x001361ac vaddr=0x40080000 size=0x00400 (  1024)
I (51177) esp_image: segment 3: paddr=0x001365b4 vaddr=0x40080400 size=0x09a64 ( 39524)
I (51197) esp_image: segment 4: paddr=0x00140020 vaddr=0x400d0020 size=0x93558 (603480) map
I (51407) esp_image: segment 5: paddr=0x001d3580 vaddr=0x40089e64 size=0x0b5cc ( 46540)
I (51427) esp_image: segment 0: paddr=0x00110020 vaddr=0x3f400020 size=0x22e58 (142936) map
I (51477) esp_image: segment 1: paddr=0x00132e80 vaddr=0x3ffb0000 size=0x03324 ( 13092)
I (51477) esp_image: segment 2: paddr=0x001361ac vaddr=0x40080000 size=0x00400 (  1024)
I (51477) esp_image: segment 3: paddr=0x001365b4 vaddr=0x40080400 size=0x09a64 ( 39524)
I (51507) esp_image: segment 4: paddr=0x00140020 vaddr=0x400d0020 size=0x93558 (603480) map
I (51707) esp_image: segment 5: paddr=0x001d3580 vaddr=0x40089e64 size=0x0b5cc ( 46540)
I (51747) task_ota: Prepare to restart system!
...

ts Jun  8 2016 00:22:57
rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:7232
ho 0 tail 12 room 4
load:0x40078000,len:14176
load:0x40080400,len:4480
entry 0x400806f0
I (75) boot: Chip Revision: 0
I (34) boot: ESP-IDF v4.1-dev-1088-gb258fc376-dirty 2nd stage bootloader
...

```

An example log when the running firmware version is same:

```
...
I (6013) task_ota: Writing to partition subtype 17 at offset 0x210000
I (6023) task_ota: New firmware version: 2
I (6023) task_ota: Running firmware version: 2
W (6023) task_ota: Current running version is the same as a new. We will not continue the update.
...
```

#### Disable `OTA`

```
> pwd
/home/trombik/github/trombik/roomPing/src
> rm update
```
