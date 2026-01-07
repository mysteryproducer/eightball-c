from machine import Pin, I2C, SPI
from imu import MPU6050
from time import sleep
from gc9a01py import GC9A01, GREEN

i2c = I2C(1)
print(i2c.scan())

mpu = MPU6050(i2c)
    
power = Pin(14,Pin.OUT)
power.value(1)

spi = SPI(1, 10_000_000)
screen=GC9A01(spi, 
#, sck=Pin(10), mosi=Pin(11),
              dc=Pin(15,Pin.OUT), cs=Pin(26,Pin.OUT), reset=Pin(27,Pin.OUT))
screen.fill(GREEN)
while(True):
    print(mpu.accel.xyz)
    sleep(.5)
