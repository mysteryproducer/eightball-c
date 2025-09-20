#define PIN_I2C_SDA 2
#define PIN_I2C_SCL 1
#define I2C_CLOCK 100000

#define PIN_SPI_CLK     8
#define PIN_SPI_MOSI    19
// #define PIN_SPI_MISO    20
#define PIN_SPI_DC      14
#define PIN_SPI_CS      15
#define PIN_SPI_RESET   18

#define PIN_TFT_POWER   7

#define LCD_RES         240
//#define TEST_LCD_V_RES              (240)
#define LCD_BIT_PER_PIXEL 16

/*
#GC9A01 screen uses SPI interface; more pins needed:
SCR_SCK=Pin(1, Pin.OUT) #clock
SCR_MOSI=Pin(2, Pin.OUT) #data
SCR_DC=Pin(3, Pin.OUT) #
SCR_CS=Pin(4, Pin.OUT) # channel select
SCR_RESET=Pin(5, Pin.OUT)
SPI_CLOCK=40000000
# This is the switch pin for the screen.
SCR_POW=6

GENERATOR="dsm5"
# MPU6050 outputs in Gs, so magnitude will be ~ 1 when still.
# Shaking will record a higher G load
TRIGGER_GS=1.3
SLEEP_TIMEOUT_MS=10000
TRIGGER_COOLDOWN=500

invert_predicate=lambda v: v[2] < -12000
def _log_message(message):
    print(message)
log_msg=lambda msg: _log_message(msg)

*/