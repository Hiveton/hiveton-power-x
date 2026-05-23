# PX1 Flash Attempt

- Time: 2026-05-19 13:55:57 +0800
- Firmware: `PowerXCode/FreeRTOS/obj/FreeRTOS.bin`
- Firmware SHA256: `65358a7a4ffd0460d2fc11149cacbb01c441962c50630ac1395add97551b6d6b`
- wchisp: `/Users/hiveton/.cargo/bin/wchisp`
- Wait seconds: `120`
- Status: `flashed`
- Message: wchisp flash completed successfully.

## Previous Flash Success

- Previous flash success report: `/Users/hiveton/HivetonCode/HivetonPowerX/artifacts/flash/latest-success.md`
- Previous flash success SHA256: `c727a47c57545ba8a523f4bf1d2b698a3e175ced8ea09a2fe4c5c0d0a7d12e5e`
- Previous flash success status: `stale`

## wchisp info

- wchisp info exit: `0`

```text
05:55:54 [INFO] Opening USB device #0
05:55:54 [INFO] Chip: CH32L103K8U6[0x3225] (Code Flash: 64KiB)
05:55:54 [INFO] Chip UID: CD-AB-7D-51-1C-BC-66-B9
05:55:54 [INFO] BTVER(bootloader ver): 02.60
05:55:54 [INFO] Current config registers: a55a3fc000ff00ffffffffff00020600cdab7d511cbc66b9
```

## wchisp flash

- wchisp flash exit: `0`

```text
05:55:54 [INFO] Opening USB device #0
05:55:54 [INFO] Chip: CH32L103K8U6[0x3225] (Code Flash: 64KiB)
05:55:54 [INFO] Chip UID: CD-AB-7D-51-1C-BC-66-B9
05:55:54 [INFO] BTVER(bootloader ver): 02.60
05:55:54 [INFO] Current config registers: a55a3fc000ff00ffffffffff00020600cdab7d511cbc66b9
05:55:54 [INFO] Read PowerXCode/FreeRTOS/obj/FreeRTOS.bin as Binary format
05:55:54 [INFO] Firmware size: 33792
05:55:54 [INFO] Erasing...
05:55:54 [INFO] Erased 34 code flash sectors
05:55:55 [INFO] Erase done
05:55:55 [INFO] Writing to code flash...
05:55:56 [INFO] Code flash 33792 bytes written
05:55:56 [INFO] Verifying...
05:55:57 [INFO] Verify OK
05:55:57 [INFO] Now reset device and skip any communication errors
05:55:57 [INFO] Device reset
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
  |       "USBSpeed" = 1
  |       "UsbLinkSpeed" = 12000000
  |       "idProduct" = 29987
  |       "iManufacturer" = 0
  |       "bDeviceClass" = 255
--
  |       "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |       "Device Speed" = 1
  |       "idVendor" = 6790
  |       "kUSBProductString" = "USB Serial"
  |       "IOGeneralInterest" = "IOCommand is not serializable"
--
    |     "USBSpeed" = 3
    |     "UsbLinkSpeed" = 480000000
    |     "idProduct" = 20654
    |     "iManufacturer" = 1
    |     "bDeviceClass" = 9
--
    |     "USB Vendor Name" = "Generic"
    |     "Device Speed" = 2
    |     "idVendor" = 10007
    |     "kUSBProductString" = "USB2.1 Hub"
    |     "IOGeneralInterest" = "IOCommand is not serializable"
--
          "USBSpeed" = 4
          "UsbLinkSpeed" = 5000000000
          "idProduct" = 20654
          "iManufacturer" = 1
          "bDeviceClass" = 9
--
          "USB Vendor Name" = "Generic"
          "Device Speed" = 3
          "idVendor" = 10007
          "kUSBProductString" = "USB3.2 Hub"
          "IOGeneralInterest" = "IOCommand is not serializable"
```

## ioreg USB Snapshot

```text
  | +-o USB Serial@01100000  <class IOUSBHostDevice, id 0x10006044c, registered, matched, active, busy 0 (93 ms), retain 35>
  |       "idProduct" = 29987
  |       "USB Product Name" = "USB Serial"
  |       "idVendor" = 6790
  |       "kUSBProductString" = "USB Serial"
    +-o USB2.1 Hub@02100000  <class IOUSBHostDevice, id 0x10005fbc8, registered, matched, active, busy 0 (116 ms), retain 35>
    |     "idProduct" = 20654
    |     "USB Product Name" = "USB2.1 Hub"
    |     "USB Vendor Name" = "Generic"
    |     "idVendor" = 10007
    |     "kUSBProductString" = "USB2.1 Hub"
    |     "kUSBVendorString" = "Generic"
    +-o USB3.2 Hub@02200000  <class IOUSBHostDevice, id 0x10005fbca, registered, matched, active, busy 0 (71 ms), retain 35>
          "idProduct" = 20654
          "USB Product Name" = "USB3.2 Hub"
          "USB Vendor Name" = "Generic"
          "idVendor" = 10007
          "kUSBProductString" = "USB3.2 Hub"
          "kUSBVendorString" = "Generic"
```

## IOUSBHostDevice Probe

```text
  |   "USBSpeed" = 1
  |   "UsbLinkSpeed" = 12000000
  |   "idProduct" = 29987
  |   "iManufacturer" = 0
  |   "bDeviceClass" = 255
--
  |   "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |   "Device Speed" = 1
  |   "idVendor" = 6790
  |   "kUSBProductString" = "USB Serial"
  |   "IOGeneralInterest" = "IOCommand is not serializable"
--
  |     "IOProviderClass" = "IOUSBHostDevice"
  |     "IOUserServerCDHash" = "35abcf2198eda08b7cfe03dd7e8542f18acca8b8"
  |     "idProduct" = 29987
  |     "IOProbeScore" = 90000
  |     "IOMatchCategory" = "IODefaultMatchCategory"
  |     "idVendor" = 6790
  |     "IOPersonalityPublisher" = "com.apple.DriverKit-AppleUSBCHCOM"
  |     "CFBundleIdentifierKernel" = "com.apple.kpi.iokit"
--
  | |   "bcdDevice" = 33076
  | |   "USBSpeed" = 1
  | |   "idProduct" = 29987
  | |   "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  | |   "bInterfaceSubClass" = 1
--
  | |   "iInterface" = 0
  | |   "bAlternateSetting" = 0
  | |   "idVendor" = 6790
  | |   "bInterfaceNumber" = 0
  | |   "bInterfaceClass" = 255
--
  |   |   "IOUserClass" = "AppleUSBCHCOM"
  |   |   "IOUserServerName" = "com.apple.DriverKit.AppleUSBCHCOM"
  |   |   "idProduct" = 29987
  |   |   "IOPersonalityPublisher" = "com.apple.DriverKit-AppleUSBCHCOM"
  |   |   "IOPowerManagement" = {"CapabilityFlags"=2,"MaxPowerState"=3,"CurrentPowerState"=3}
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
--
  |   "USBSpeed" = 3
  |   "UsbLinkSpeed" = 480000000
  |   "idProduct" = 20654
  |   "iManufacturer" = 1
  |   "bDeviceClass" = 9
--
  |   "USB Vendor Name" = "Generic"
  |   "Device Speed" = 2
  |   "idVendor" = 10007
  |   "kUSBProductString" = "USB2.1 Hub"
  |   "IOGeneralInterest" = "IOCommand is not serializable"
--
  |     "bcdDevice" = 305
  |     "USBSpeed" = 3
  |     "idProduct" = 20654
  |     "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |     "bInterfaceSubClass" = 0
--
  |     "iInterface" = 0
  |     "bAlternateSetting" = 1
  |     "idVendor" = 10007
  |     "bInterfaceNumber" = 0
  |     "bInterfaceClass" = 9
--
  |   "USBSpeed" = 4
  |   "UsbLinkSpeed" = 5000000000
  |   "idProduct" = 20654
  |   "iManufacturer" = 1
  |   "bDeviceClass" = 9
--
  |   "USB Vendor Name" = "Generic"
  |   "Device Speed" = 3
  |   "idVendor" = 10007
  |   "kUSBProductString" = "USB3.2 Hub"
  |   "IOGeneralInterest" = "IOCommand is not serializable"
--
  |     "bcdDevice" = 305
  |     "USBSpeed" = 4
  |     "idProduct" = 20654
  |     "IOServiceDEXTEntitlements" = (("com.apple.developer.driverkit.transport.usb"))
  |     "bInterfaceSubClass" = 0
--
  |     "iInterface" = 0
  |     "bAlternateSetting" = 0
  |     "idVendor" = 10007
  |     "bInterfaceNumber" = 0
  |     "bInterfaceClass" = 9
```

## IOUSBHostDevice Snapshot

```text
+-o USB Serial@01100000  <class IOUSBHostDevice, id 0x10006044c, registered, matched, active, busy 0 (93 ms), retain 35>
  |   "idProduct" = 29987
  |   "USB Product Name" = "USB Serial"
  |   "idVendor" = 6790
  |   "kUSBProductString" = "USB Serial"
  +-o AppleUSBHostCompositeDevice  <class AppleUSBHostCompositeDevice, id 0x10006044e, !registered, !matched, active, busy 0, retain 4>
  |     "idProduct" = 29987
  |     "idVendor" = 6790
  +-o IOUSBHostInterface@0  <class IOUSBHostInterface, id 0x10006044f, registered, matched, active, busy 0 (22 ms), retain 13>
  | |   "idProduct" = 29987
  | |   "USB Product Name" = "USB Serial"
  | |   "idVendor" = 6790
  | +-o AppleUSBCHCOM  <class IOUserSerial, id 0x100060455, registered, matched, active, busy 0 (1 ms), retain 14>
  |   |   "idProduct" = 29987
  |   |   "idVendor" = 6790
+-o USB2.1 Hub@02100000  <class IOUSBHostDevice, id 0x10005fbc8, registered, matched, active, busy 0 (116 ms), retain 35>
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
+-o USB3.2 Hub@02200000  <class IOUSBHostDevice, id 0x10005fbca, registered, matched, active, busy 0 (71 ms), retain 35>
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

A WCH/CH32-like USB entry is present in the probe output.

## ISP Entry Notes

- Expected WCH ISP USB IDs: `4348:55e0` or `1a86:55e0`.
- PX1 schematic labels `BTN2 (ISP)` on `PB9/BOOT0`; `PB2/BOOT1` is also present on the MCU.
- Manual entry: hold `BTN2/BOOT` while reconnecting USB, then run this script while the device remains in ISP mode.
- Keep `BTN2/BOOT` pressed before USB insertion, hold for 1-2 seconds after insertion, then release.
- If no USB VID/PID device is visible, test a known data-capable USB-C cable and direct Mac port before debugging firmware.
- If another USB device appears but not WCH ISP, re-check BOOT0 level on `PB9/BOOT0`, reset timing, and whether the MCU is entering user firmware instead of Bootloader.
