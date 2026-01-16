# This file is executed on every boot (including wake-boot from deepsleep)
#import esp
#esp.osdebug(None)
#import webrepl
#webrepl.start()

from imu import MPU6050
from machine import Pin, I2C

i2c = I2C(0,sda=Pin(6,Pin.OUT),scl=Pin(5,Pin.OUT))
mpu = MPU6050(i2c)
mpu.activate_motion_detect()

print(mpu.accel.xyz)