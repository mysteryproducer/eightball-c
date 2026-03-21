Magic 8 Ball project, the C++ version.
This was written to save power on operation, but it has new features too!

Setup for VS Code:
1. Install ESP-IDF: [instructions here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation)
2. Install ESP-IDF extension for VS Code
3. Clone the code!
4. Select ESP-IDF version, target chip, and port.

TODO: creating a FAT image with cmake seems to produce a disk that can't be mounted by a remote host. A couple of working images are provided, but I need to check what state they're in.

Platform-dependent features:
When building for ESP32-S3, a 2-second window opens for mounting the FAT image over USB. 
TODO: unmount event doesn't fire and the machine needs a hard reset to exit mass storage mode. 
