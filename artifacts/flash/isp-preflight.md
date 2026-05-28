# PX1 WCH ISP Preflight

- Time: 2026-05-23 14:17:08 +0800
- Status: `failed`
- Message: wchisp not found; install it or set WCHISP=/path/to/wchisp.
- Wait seconds: `0`
- wchisp: `not found`

## wchisp info

- wchisp info exit: `127`

```text
wchisp not found
```

## system_profiler USB Probe

```text
No WCH/CH32 ISP USB match found in system_profiler.
```

## system_profiler USB Snapshot

```text
### SPUSBHostDataType
USB:
    USB 3.1 Bus:
      Location ID: 0x02000000
      Connection Type: Built-in
      Driver: AppleT8132USBXHCI
        USB3 Gen2 Hub:
          Location ID: 0x02200000
          Connection Type: Built-in
          Manufacturer: Apple
          Serial Number: 7423J07
          Link Speed: 10 Gb/s
          USB Vendor ID: 0x05ac
          USB Product ID: 0x800c
          USB Product Version: 0x5615
        USB2 Hub:
          Location ID: 0x02100000
          Connection Type: Built-in
          Manufacturer: Apple
          Serial Number: 7423J07
          Link Speed: 480 Mb/s
          USB Vendor ID: 0x05ac
          USB Product ID: 0x800b
          USB Product Version: 0x5615
    USB 3.1 Bus:
      Location ID: 0x01000000
      Connection Type: Built-in
      Driver: AppleT8132USBXHCI
    USB 3.1 Bus:
      Location ID: 0x00000000
      Connection Type: Built-in
      Driver: AppleT8132USBXHCI
    USB 3.1 Bus:
      Location ID: 0x03000000
      Connection Type: Built-in
      Driver: AppleT8132USBXHCI
        USB 2.0 Hub:
          Location ID: 0x03100000
          Connection Type: Removable
          Serial Number: Not Provided
          Link Speed: 480 Mb/s
          USB Vendor ID: 0x1a40
          USB Product ID: 0x0801
          USB Product Version: 0x0100
            Rapoo 2.4G Wireless Device:
              Location ID: 0x03120000
              Connection Type: Removable
              Manufacturer: RAPOO
              Serial Number: Not Provided
              Link Speed: 12 Mb/s
              USB Vendor ID: 0x24ae
              USB Product ID: 0x2013
              USB Product Version: 0x0110
            Bluetooth Keyboard:
              Location ID: 0x03110000
              Connection Type: Removable
              Manufacturer: BY Tech
              Serial Number: Not Provided
              Link Speed: 12 Mb/s
              USB Vendor ID: 0x05ac
              USB Product ID: 0x024f
              USB Product Version: 0x0102
              Power Allocated: 2.5 W (500 mA)
            Redmi 电脑音箱:
              Location ID: 0x03130000
              Connection Type: Removable
              Manufacturer: MV-SILICON
              Serial Number: 20190808
              Link Speed: 12 Mb/s
              USB Vendor ID: 0x2717
              USB Product ID: 0x5086
              USB Product Version: 0x0100
```

## ioreg USB Probe

```text
No WCH/CH32 ISP USB match found in ioreg.
```

## ioreg USB Snapshot

```text
  | +-o USB3 Gen2 Hub@02200000  <class IOUSBHostDevice, id 0x100000a41, registered, matched, active, busy 0 (8 ms), retain 27>
  | |     "USB Vendor Name" = "Apple"
  | |     "kUSBProductString" = "USB3 Gen2 Hub"
  | |     "kUSBVendorString" = "Apple"
  | |     "USB Product Name" = "USB3 Gen2 Hub"
  | |     "idVendor" = 1452
  | |     "idProduct" = 32780
  | +-o USB2 Hub@02100000  <class IOUSBHostDevice, id 0x100000a53, registered, matched, active, busy 0 (8 ms), retain 27>
  |       "USB Vendor Name" = "Apple"
  |       "kUSBProductString" = "USB2 Hub"
  |       "kUSBVendorString" = "Apple"
  |       "USB Product Name" = "USB2 Hub"
  |       "idVendor" = 1452
  |       "idProduct" = 32779
    +-o USB 2.0 Hub@03100000  <class IOUSBHostDevice, id 0x10009f040, registered, matched, active, busy 0 (187 ms), retain 35>
      |   "idProduct" = 2049
      |   "USB Product Name" = "USB 2.0 Hub"
      |   "idVendor" = 6720
      |   "kUSBProductString" = "USB 2.0 Hub"
      +-o Rapoo 2.4G Wireless Device@03120000  <class IOUSBHostDevice, id 0x10009f052, registered, matched, active, busy 0 (57 ms), retain 38>
      |     "idProduct" = 8211
      |     "USB Product Name" = "Rapoo 2.4G Wireless Device"
      |     "USB Vendor Name" = "RAPOO"
      |     "idVendor" = 9390
      |     "kUSBProductString" = "Rapoo 2.4G Wireless Device"
      |     "kUSBVendorString" = "RAPOO"
      +-o Bluetooth Keyboard@03110000  <class IOUSBHostDevice, id 0x10009f06e, registered, matched, active, busy 0 (47 ms), retain 33>
      |     "idProduct" = 591
      |     "USB Product Name" = "Bluetooth Keyboard"
      |     "USB Vendor Name" = "BY Tech"
      |     "idVendor" = 1452
      |     "kUSBProductString" = "Bluetooth Keyboard"
      |     "kUSBVendorString" = "BY Tech"
      +-o Redmi 电脑音箱@03130000  <class IOUSBHostDevice, id 0x10009f0a3, registered, matched, active, busy 0 (73 ms), retain 34>
            "idProduct" = 20614
            "USB Product Name" = 
            "USB Vendor Name" = "MV-SILICON"
            "idVendor" = 10007
            "kUSBProductString" = 
            "kUSBVendorString" = "MV-SILICON"
```

## IOUSBHostDevice Probe

```text
No WCH/CH32 ISP USB match found in IOUSBHostDevice.
```

## IOUSBHostDevice Snapshot

```text
+-o USB2 Hub@02100000  <class IOUSBHostDevice, id 0x100000a53, registered, matched, active, busy 0 (8 ms), retain 27>
  |   "USB Vendor Name" = "Apple"
  |   "kUSBProductString" = "USB2 Hub"
  |   "kUSBVendorString" = "Apple"
  |   "USB Product Name" = "USB2 Hub"
  |   "idVendor" = 1452
  |   "idProduct" = 32779
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x100000a57, !registered, !matched, active, busy 0, retain 8>
  |     "idProduct" = 32779
  |     "USB Product Name" = "USB2 Hub"
  |     "USB Vendor Name" = "Apple"
  |     "idVendor" = 1452
+-o USB3 Gen2 Hub@02200000  <class IOUSBHostDevice, id 0x100000a41, registered, matched, active, busy 0 (8 ms), retain 27>
  |   "USB Vendor Name" = "Apple"
  |   "kUSBProductString" = "USB3 Gen2 Hub"
  |   "kUSBVendorString" = "Apple"
  |   "USB Product Name" = "USB3 Gen2 Hub"
  |   "idVendor" = 1452
  |   "idProduct" = 32780
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x100000a4e, !registered, !matched, active, busy 0, retain 8>
  |     "idProduct" = 32780
  |     "USB Product Name" = "USB3 Gen2 Hub"
  |     "USB Vendor Name" = "Apple"
  |     "idVendor" = 1452
+-o USB 2.0 Hub@03100000  <class IOUSBHostDevice, id 0x10009f040, registered, matched, active, busy 0 (187 ms), retain 35>
  |   "idProduct" = 2049
  |   "USB Product Name" = "USB 2.0 Hub"
  |   "idVendor" = 6720
  |   "kUSBProductString" = "USB 2.0 Hub"
  | | +-o Bluetooth Keyboard@03110000  <class IOUSBHostDevice, id 0x10009f06e, registered, matched, active, busy 0 (47 ms), retain 33>
  | |   |   "idProduct" = 591
  | |   |   "USB Product Name" = "Bluetooth Keyboard"
  | |   |   "USB Vendor Name" = "BY Tech"
  | |   |   "idVendor" = 1452
  | |   |   "kUSBProductString" = "Bluetooth Keyboard"
  | |   |   "kUSBVendorString" = "BY Tech"
  | |   +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10009f07e, registered, matched, active, busy 0 (40 ms), retain 11>
  | |   | |   "idProduct" = 591
  | |   | |   "USB Product Name" = "Bluetooth Keyboard"
  | |   | |   "USB Vendor Name" = "BY Tech"
  | |   | |   "idVendor" = 1452
  | |   +-o IOUSBHostInterface@1  <class IOUSBHostInterface, id 0x10009f081, registered, matched, active, busy 0 (31 ms), retain 11>
  | |   | |   "idProduct" = 591
  | |   | |   "USB Product Name" = "Bluetooth Keyboard"
  | |   | |   "USB Vendor Name" = "BY Tech"
  | |   | |   "idVendor" = 1452
  | | +-o Rapoo 2.4G Wireless Device@03120000  <class IOUSBHostDevice, id 0x10009f052, registered, matched, active, busy 0 (57 ms), retain 38>
  | |   |   "idProduct" = 8211
  | |   |   "USB Product Name" = "Rapoo 2.4G Wireless Device"
  | |   |   "USB Vendor Name" = "RAPOO"
  | |   |   "idVendor" = 9390
  | |   |   "kUSBProductString" = "Rapoo 2.4G Wireless Device"
  | |   |   "kUSBVendorString" = "RAPOO"
  | |   +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10009f058, registered, matched, active, busy 0 (38 ms), retain 11>
  | |   | |   "idProduct" = 8211
  | |   | |   "USB Product Name" = "Rapoo 2.4G Wireless Device"
  | |   | |   "USB Vendor Name" = "RAPOO"
  | |   | |   "idVendor" = 9390
  | |   +-o IOUSBHostInterface@1  <class IOUSBHostInterface, id 0x10009f05b, registered, matched, active, busy 0 (45 ms), retain 11>
  | |   | |   "idProduct" = 8211
  | |   | |   "USB Product Name" = "Rapoo 2.4G Wireless Device"
  | |   | |   "USB Vendor Name" = "RAPOO"
  | |   | |   "idVendor" = 9390
  | |   +-o IOUSBHostInterface@2  <class IOUSBHostInterface, id 0x10009f05d, registered, matched, active, busy 0 (40 ms), retain 11>
  | |     |   "idProduct" = 8211
  | |     |   "USB Product Name" = "Rapoo 2.4G Wireless Device"
  | |     |   "USB Vendor Name" = "RAPOO"
  | |     |   "idVendor" = 9390
  | | +-o Redmi 电脑音箱@03130000  <class IOUSBHostDevice, id 0x10009f0a3, registered, matched, active, busy 0 (73 ms), retain 34>
  | |   |   "idProduct" = 20614
  | |   |   "USB Product Name" = 
  | |   |   "USB Vendor Name" = "MV-SILICON"
  | |   |   "idVendor" = 10007
  | |   |   "kUSBProductString" = 
  | |   |   "kUSBVendorString" = "MV-SILICON"
  | |   +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10009f0ab, registered, matched, active, busy 0 (14 ms), retain 10>
  | |   | |   "idProduct" = 20614
  | |   | |   "USB Product Name" = 
  | |   | |   "USB Vendor Name" = "MV-SILICON"
  | |   | |   "idVendor" = 10007
  | |   +-o IOUSBHostInterface@1  <class IOUSBHostInterface, id 0x10009f0ac, registered, matched, active, busy 0 (7 ms), retain 9>
  | |   | |   "idProduct" = 20614
  | |   | |   "USB Product Name" = 
  | |   | |   "USB Vendor Name" = "MV-SILICON"
  | |   | |   "idVendor" = 10007
  | |   +-o IOUSBHostInterface@2  <class IOUSBHostInterface, id 0x10009f0ad, registered, matched, active, busy 0 (5 ms), retain 9>
  | |   | |   "idProduct" = 20614
  | |   | |   "USB Product Name" = 
  | |   | |   "USB Vendor Name" = "MV-SILICON"
  | |   | |   "idVendor" = 10007
  | |   +-o IOUSBHostInterface@3  <class IOUSBHostInterface, id 0x10009f0b0, registered, matched, active, busy 0 (50 ms), retain 11>
  | |   | |   "idProduct" = 20614
  | |   | |   "USB Product Name" = 
  | |   | |   "USB Vendor Name" = "MV-SILICON"
  | |   | |   "idVendor" = 10007
  | |   +-o IOUSBHostInterface@4  <class IOUSBHostInterface, id 0x10009f0b1, registered, matched, active, busy 0 (26 ms), retain 6>
  | |         "idProduct" = 20614
  | |         "USB Product Name" = 
  | |         "USB Vendor Name" = "MV-SILICON"
  | |         "idVendor" = 10007
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10009f043, !registered, !matched, active, busy 0 (0 ms), retain 8>
  |     "idProduct" = 2049
  |     "USB Product Name" = "USB 2.0 Hub"
  |     "idVendor" = 6720
```

## Serial Ports

```text
/dev/cu.Bluetooth-Incoming-Port
/dev/cu.debug-console
```

## Thunderbolt/USB4 Port Snapshot

```text
Thunderbolt/USB4:
    Thunderbolt/USB4 Bus 3:
      Vendor Name: Apple Inc.
      Device Name: iOS
      UID: 0x05AC548A1F036493
      Route String: 0
      Domain UUID: 36FB56D6-9A9A-4437-9069-B782FB8601DB
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 40 Gb/s
          Receptacle: 4
    Thunderbolt/USB4 Bus 1:
      Vendor Name: Apple Inc.
      Device Name: iOS
      UID: 0x05AC548A1F036491
      Route String: 0
      Domain UUID: 8BCC5182-4757-4CF7-9169-9FAA1C087C9F
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 40 Gb/s
          Receptacle: 2
    Thunderbolt/USB4 Bus 0:
      Vendor Name: Apple Inc.
      Device Name: iOS
      UID: 0x05AC548A1F036490
      Route String: 0
      Domain UUID: 9C94FDD0-0737-4ACA-AB07-EA430099168F
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 40 Gb/s
          Receptacle: 1
```

## USB Host Diagnosis

USB devices may be present, but no WCH/CH32 ISP VID/PID was found.

## ISP Entry Notes

- Expected WCH ISP USB IDs: `4348:55e0` or `1a86:55e0`.
- PX1 schematic labels `BTN2 (ISP)` on `PB9/BOOT0`; `PB2/BOOT1` is also present on the MCU.
- Manual entry: hold `BTN2/BOOT` while reconnecting USB, then run this script again before flashing.
- Keep `BTN2/BOOT` pressed before USB insertion, hold for 1-2 seconds after insertion, then release.
- If no USB VID/PID device is visible, test a known data-capable USB-C cable and direct Mac port before debugging firmware.
- If another USB device appears but not WCH ISP, re-check BOOT0 level on `PB9/BOOT0`, reset timing, and whether the MCU is entering user firmware instead of Bootloader.
