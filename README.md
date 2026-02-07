Magic 8 Ball project, the C++ version.
This was written to save power on operation.

Setup for VS Code:
1. Install ESP-IDF: [instructions here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation)
2. Install ESP-IDF extension for VS Code
3. Clone the code!
4. Select ESP-IDF version, target chip, and port.
5. Open ESP-IDF project settings. In 'Serial Flasher Config', select 'Partition Table' option for custom CSV. The default value for the CSV file, partitions.csv, is the correct filename.
6. Build will probably fail! in managed_components/espressif__mpu6050/CMakeLists.txt, add a dependency on line 2 for "driver"
