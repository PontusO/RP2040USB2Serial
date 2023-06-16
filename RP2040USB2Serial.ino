/*
  Challenger USB to serial port.

  This is an example on how to flash an onboard ESP8285 or ESP32C3.
  TinyUSB must be selected for this sketch to work.

  The ESP8285 needs to flashed with the following command:
    esptool.py --port <port> --baud <rate> write_flash -e -fm dout -fs 1MB <filename.bin>

  This program must be compiled with tinyUSB to work.
  
*/
#include <Arduino.h>
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#define USE_OVERRIDE_LINE_STATE_CB    1
#else
#warning "Using Pico SDK USB stack, can not flash ESP-device !"
#endif
#include <ChallengerWiFi.h>

// Sets the level of debug info that is printed on the serial output
//   0 = No debug information at all
//   1 = Basic debug information
//   2 = Full debug information (Will make the download very slow)
#define DEBUG_LVL     0

#define PC_PORT       Serial
#define DEBUG_PORT    Serial1
#define PACKET_SIZE   64

#define digitalToggle(pin)  digitalWrite(pin, !digitalRead(pin))

int led = LED_BUILTIN;
uint32_t baud_rate = 115200;
static int port = -1;
static int words = 0;
uint8_t ch;
char buffer[PACKET_SIZE];
size_t received;
size_t available;
uint32_t led_timer;

#if defined(USE_OVERRIDE_LINE_STATE_CB)
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* coding) {
  (void) itf;
  if (coding->bit_rate != baud_rate) {

    // flush and change baudrate to ESP8285/ESP32-C3/ESP32-C6
    ESP_SERIAL_PORT.begin(coding->bit_rate);
    baud_rate = coding->bit_rate;
#if DEBUG_LVL >= 1
    DEBUG_PORT.printf("Setting new baudrate to %d\r\n", baud_rate);
#endif    
  }
}

//
// This overrides the stack functionality to capture cdc line state
// changes.
//
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;  // interface ID, not used
  digitalWrite(led, LOW);

  if (rts) {
    digitalWrite(PIN_ESP_RST, HIGH);
#if DEBUG_LVL >= 1
    DEBUG_PORT.printf("RST=HIGH\r\n");
#endif    
  } else {
    digitalWrite(PIN_ESP_RST, LOW);
#if DEBUG_LVL >= 1
    DEBUG_PORT.printf("RST=LOW\r\n");
#endif    
  }
    
  if (dtr) {
    digitalWrite(PIN_ESP_MODE, HIGH);
#if DEBUG_LVL >= 1
    DEBUG_PORT.printf("MODE=HIGH\r\n");
#endif    
  } else {
    digitalWrite(PIN_ESP_MODE, LOW);
#if DEBUG_LVL >= 1
    DEBUG_PORT.printf("MODE=LOW\r\n");
#endif    
  }

  // DTR = false is counted as disconnected
  if (!dtr) {
    // touch1200 only with first CDC instance (Serial)
    if (itf == 0) {
      cdc_line_coding_t coding;
      tud_cdc_get_line_coding(&coding);

      if (coding.bit_rate == 1200) {
        TinyUSB_Port_EnterDFU();
      }
    }
  }
}
#endif

// the setup routine runs once when you press reset:
void setup()
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  pinMode(D15, OUTPUT);
  digitalWrite(D15, HIGH);

  pinMode(PIN_ESP_RST, OUTPUT);
  digitalWrite(PIN_ESP_RST, HIGH);
  pinMode(PIN_ESP_MODE, OUTPUT);
  digitalWrite(PIN_ESP_MODE, HIGH);
  
#if DEBUG_LVL >= 1
  DEBUG_PORT.begin(921600);
  DEBUG_PORT.println("\n\nDebug monitor !");
#endif

  // Set initial state of control pins
  // Normal start when the reset pin is released
  Challenger2040WiFi.doHWReset();

  while (!PC_PORT)
    delay(10);
    
  // Set default baudrates
  PC_PORT.begin(baud_rate);

  ESP_SERIAL_PORT.setTimeout(100);
  PC_PORT.setTimeout(100);
  
  digitalWrite(led, LOW);
}

//
// The main loop were data is received from the host and then forwarded
// to the ESP8285/ESP32C3 and vice versa.
//
void loop() {
  // Handle data from the USB port ment for the ESP8285/ESP32-C3/ESP32-C6
  available = PC_PORT.available();
  if (available) {
    received = PC_PORT.readBytes(buffer, PACKET_SIZE);
    ESP_SERIAL_PORT.write(buffer, received);

#if DEBUG_LVL >= 1
    if (port != 1) {
      port = 1;
      words = 0;
      DEBUG_PORT.printf("Transfering %d bytes from USB to ESP8285/ESP32-C3/ESP32-C6 !\r\n", received);
    }
#if DEBUG_LVL >= 2
    for (int i=0;i<received;i++) {
      DEBUG_PORT.printf("%02x ", buffer[i]);
      if (++words > 15) {
        words = 0;
        DEBUG_PORT.println();
      }
    }
    DEBUG_PORT.println();    
#endif
#endif
  }

  // Handle data from the ESP8285/ESP32-C3/ESP32-C6 port meant for the USB port
  available = ESP_SERIAL_PORT.available();
  if (available) {
    bool final_chars_written = false;
    int i;

    received = ESP_SERIAL_PORT.readBytes(buffer, available);
    PC_PORT.write(buffer, received);
    PC_PORT.flush();

#if DEBUG_LVL >= 1
    if (port != 1) {
      port = 1;
      words = 0;
      DEBUG_PORT.printf("Transfering %d bytes from ESP8285/ESP32-C3/ESP32-C6 to USB !\r\n", received);
    }
#if DEBUG_LVL >= 2
    char chars[16];
    int cptr = 0;
    for (i=0;i<received;i++) {
      DEBUG_PORT.printf("%02x ", buffer[i]);
      if (isalnum(buffer[i]))
        chars[cptr] = buffer[i];
      else
        chars[cptr] = '?';
      cptr++;
      if (++words > 15) {
        words = 0;
        for (int j=0;j<(48-((i&15)*3));j++)
          DEBUG_PORT.write(' ');
        for (int j=0;j<cptr;j++)
          DEBUG_PORT.printf("%c ", chars[j]);
        DEBUG_PORT.println();
        final_chars_written = true;
        cptr=0;
      }
    }
    words = 0;
    if (!final_chars_written) {
      for (int j=0;j<(48-((i&15)*3)+3);j++)
        DEBUG_PORT.write(' ');
      for (int j=0;j<cptr;j++)
        DEBUG_PORT.printf("%c ", chars[j]);      
      DEBUG_PORT.println();
    } else {
      DEBUG_PORT.println();
    }
#endif
#endif
  }
}
