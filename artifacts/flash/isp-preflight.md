# PX1 WCH ISP Preflight

- Time: 2026-05-19 11:40:17 +0800
- Status: `no-isp-device`
- Message: No WCH ISP USB device found; expected 4348:55e0 or 1a86:55e0.
- Wait seconds: `0`
- wchisp: `/Users/hiveton/.cargo/bin/wchisp`

## wchisp info

- wchisp info exit: `1`

```text
03:40:16 [INFO] Opening USB device #0
Error: No WCH ISP USB device found(4348:55e0 or 1a86:55e0 device not found at index #0)
```

## system_profiler USB Probe

```text
          Serial Number: Not Provided
          Link Speed: 12 Mb/s
          USB Vendor ID: 0x1a86
          USB Product ID: 0x7523
          USB Product Version: 0x8134
```

## system_profiler USB Snapshot

```text
### SPUSBHostDataType
USB:
    USB 4.0 Bus:
      Location ID: 0x08000000
      Connection Type: Built-in
      Driver: AppleT6050USBXHCIAUSS
    USB 3.1 Bus:
      Location ID: 0x01000000
      Connection Type: Built-in
      Driver: AppleT8142USBXHCI
        USB Serial:
          Location ID: 0x01100000
          Connection Type: Removable
          Serial Number: Not Provided
          Link Speed: 12 Mb/s
          USB Vendor ID: 0x1a86
          USB Product ID: 0x7523
          USB Product Version: 0x8134
          Power Allocated: 0.52 W (104 mA)
    USB 3.1 Bus:
      Location ID: 0x00000000
      Connection Type: Built-in
      Driver: AppleT8142USBXHCI
    USB 3.1 Bus:
      Location ID: 0x02000000
      Connection Type: Built-in
      Driver: AppleT8142USBXHCI
        USB2.1 Hub:
          Location ID: 0x02100000
          Connection Type: Removable
          Manufacturer: Generic
          Serial Number: Not Provided
          Link Speed: 480 Mb/s
          USB Vendor ID: 0x2717
          USB Product ID: 0x50ae
          USB Product Version: 0x0131
        USB3.2 Hub:
          Location ID: 0x02200000
          Connection Type: Removable
          Manufacturer: Generic
          Serial Number: Not Provided
          Link Speed: 5 Gb/s
          USB Vendor ID: 0x2717
          USB Product ID: 0x50ae
          USB Product Version: 0x0131
```

## ioreg USB Probe

```text
  |       "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |       "Device Speed" = 1
  |       "idVendor" = 6790
  |       "kUSBProductString" = "USB Serial"
  |       "IOGeneralInterest" = "IOCommand is not serializable"
```

## ioreg USB Snapshot

```text
  | +-o USB Serial@01100000  <class IOUSBHostDevice, id 0x10005fef7, registered, matched, active, busy 0 (34 ms), retain 31>
  |       "idProduct" = 29987
  |       "USB Product Name" = "USB Serial"
  |       "idVendor" = 6790
  |       "kUSBProductString" = "USB Serial"
    +-o USB2.1 Hub@02100000  <class IOUSBHostDevice, id 0x10005fbc8, registered, matched, active, busy 0 (30 ms), retain 35>
    |     "idProduct" = 20654
    |     "USB Product Name" = "USB2.1 Hub"
    |     "USB Vendor Name" = "Generic"
    |     "idVendor" = 10007
    |     "kUSBProductString" = "USB2.1 Hub"
    |     "kUSBVendorString" = "Generic"
    +-o USB3.2 Hub@02200000  <class IOUSBHostDevice, id 0x10005fbca, registered, matched, active, busy 0 (33 ms), retain 35>
          "idProduct" = 20654
          "USB Product Name" = "USB3.2 Hub"
          "USB Vendor Name" = "Generic"
          "idVendor" = 10007
          "kUSBProductString" = "USB3.2 Hub"
          "kUSBVendorString" = "Generic"
```

## IOUSBHostDevice Probe

```text
  |   "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |   "Device Speed" = 1
  |   "idVendor" = 6790
  |   "kUSBProductString" = "USB Serial"
  |   "IOGeneralInterest" = "IOCommand is not serializable"
--
  |     "IOProbeScore" = 90000
  |     "IOMatchCategory" = "IODefaultMatchCategory"
  |     "idVendor" = 6790
  |     "IOPersonalityPublisher" = "com.apple.DriverKit-AppleUSBCHCOM"
  |     "CFBundleIdentifierKernel" = "com.apple.kpi.iokit"
--
  | |   "iInterface" = 0
  | |   "bAlternateSetting" = 0
  | |   "idVendor" = 6790
  | |   "bInterfaceNumber" = 0
  | |   "bInterfaceClass" = 255
--
  |   |   "IOProbeScore" = 89999
  |   |   "IOUserServerCDHash" = "35abcf2198eda08b7cfe03dd7e8542f18acca8b8"
  |   |   "IOMatchedPersonality" = {"IOClass"="IOUserSerial","CFBundleIdentifier"="com.apple.DriverKit-AppleUSBCHCOM","IOProviderClass"="IOUSBHostInterface","IOUserServerCDHash"="35abcf2198eda08b7cfe03dd7e8542f18acca8b8","idProduct"=29987,"bConfigurationValue"=1,"IOUserServerName"="com.apple.DriverKit.AppleUSBCHCOM","idVendor"=6790,"IOPersonalityPublisher"="com.apple.DriverKit-AppleUSBCHCOM","CFBundleIdentifierKernel"="com.apple.driver.driverkit.serial","kOSBundleDextUniqueIdentifier"=<b516df71b274189db0be8c2bd815cdeae43a0e7cd0638e25b1d328fef54b8e9d>,"bInterfaceNumber"=0,"IOUserClass"="AppleUSBCHCOM"}
  |   |   "IOTTYBaseName" = "usbserial-"
  |   |   "IOUserClasses" = ("AppleUSBCHCOM","IOUserUSBSerial","IOUserSerial","IOService","OSObject")
--
  |   |   "CFBundleIdentifier" = "com.apple.DriverKit-AppleUSBCHCOM"
  |   |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
  |   |   "idVendor" = 6790
  |   |   "IOMatchCategory" = "IODefaultMatchCategory"
  |   |   "IOTTYSuffix" = "110"
```

## IOUSBHostDevice Snapshot

```text
+-o USB Serial@01100000  <class IOUSBHostDevice, id 0x10005fef7, registered, matched, active, busy 0 (34 ms), retain 31>
  |   "idProduct" = 29987
  |   "USB Product Name" = "USB Serial"
  |   "idVendor" = 6790
  |   "kUSBProductString" = "USB Serial"
  +-o AppleUSBHostCompositeDevice  <class AppleUSBHostCompositeDevice, id 0x10005fef9, !registered, !matched, active, busy 0, retain 4>
  |     "idProduct" = 29987
  |     "idVendor" = 6790
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10005fefa, registered, matched, active, busy 0 (24 ms), retain 11>
  | |   "idProduct" = 29987
  | |   "USB Product Name" = "USB Serial"
  | |   "idVendor" = 6790
  | +-o AppleUSBCHCOM  <class IOUserSerial, id 0x10005ff00, registered, matched, active, busy 0 (1 ms), retain 11>
  |   |   "idProduct" = 29987
  |   |   "idVendor" = 6790
+-o USB2.1 Hub@02100000  <class IOUSBHostDevice, id 0x10005fbc8, registered, matched, active, busy 0 (30 ms), retain 35>
  |   "idProduct" = 20654
  |   "USB Product Name" = "USB2.1 Hub"
  |   "USB Vendor Name" = "Generic"
  |   "idVendor" = 10007
  |   "kUSBProductString" = "USB2.1 Hub"
  |   "kUSBVendorString" = "Generic"
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10005fbcd, !registered, !matched, active, busy 0 (0 ms), retain 8>
  |     "USB Vendor Name" = "Generic"
  |     "idProduct" = 20654
  |     "USB Product Name" = "USB2.1 Hub"
  |     "idVendor" = 10007
+-o USB3.2 Hub@02200000  <class IOUSBHostDevice, id 0x10005fbca, registered, matched, active, busy 0 (33 ms), retain 35>
  |   "idProduct" = 20654
  |   "USB Product Name" = "USB3.2 Hub"
  |   "USB Vendor Name" = "Generic"
  |   "idVendor" = 10007
  |   "kUSBProductString" = "USB3.2 Hub"
  |   "kUSBVendorString" = "Generic"
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10005fbd1, !registered, !matched, active, busy 0 (0 ms), retain 8>
  |     "USB Vendor Name" = "Generic"
  |     "idProduct" = 20654
  |     "USB Product Name" = "USB3.2 Hub"
  |     "idVendor" = 10007
```

## Serial Ports

```text
/dev/cu.Bluetooth-Incoming-Port
/dev/cu.debug-console
/dev/cu.usbserial-110
```

## Thunderbolt/USB4 Port Snapshot

```text
Thunderbolt/USB4:
    Thunderbolt/USB4 Bus 2:
      Vendor Name: Apple Inc.
      Device Name: MacBook Pro
      UID: 0x05AC90B6F8704662
      Route String: 0
      Domain UUID: 45C9DA81-EB67-41A9-B982-3B1B444326C9
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 120 Gb/s
          Receptacle: 3
    Thunderbolt/USB4 Bus 1:
      Vendor Name: Apple Inc.
      Device Name: MacBook Pro
      UID: 0x05AC90B6F8704661
      Route String: 0
      Domain UUID: 55E96ABC-F334-4D61-8B5D-8E7ACE856EC4
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 120 Gb/s
          Receptacle: 2
    Thunderbolt/USB4 Bus 0:
      Vendor Name: Apple Inc.
      Device Name: MacBook Pro
      UID: 0x05AC90B6F8704660
      Route String: 0
      Domain UUID: E764559B-B393-401E-B7D6-D41CEF6E1A81
      Port:
          Status: No device connected
          Link Status: 0x100
          Speed: Up to 120 Gb/s
          Receptacle: 1
```

## USB Host Diagnosis

A WCH USB serial device is visible, but no WCH ISP product ID 0x55e0 was found.

## ISP Entry Notes

- Expected WCH ISP USB IDs: `4348:55e0` or `1a86:55e0`.
- PX1 schematic labels `BTN2 (ISP)` on `PB9/BOOT0`; `PB2/BOOT1` is also present on the MCU.
- Manual entry: hold `BTN2/BOOT` while reconnecting USB, then run this script again before flashing.
- Keep `BTN2/BOOT` pressed before USB insertion, hold for 1-2 seconds after insertion, then release.
- If no USB VID/PID device is visible, test a known data-capable USB-C cable and direct Mac port before debugging firmware.
- If another USB device appears but not WCH ISP, re-check BOOT0 level on `PB9/BOOT0`, reset timing, and whether the MCU is entering user firmware instead of Bootloader.
