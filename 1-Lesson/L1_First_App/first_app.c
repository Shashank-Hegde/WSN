#include "contiki.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include <string.h>

PROCESS(first_process, "Main process of the first WSN lab application");

AUTOSTART_PROCESSES(&first_process);

PROCESS_THREAD(first_process, ev, data)
{
  PROCESS_BEGIN();

  button_sensor.configure(BUTTON_SENSOR_CONFIG_TYPE_INTERVAL, 4 * CLOCK_SECOND);

  leds_off(LEDS_ALL);

  while (1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == sensors_event)
    {
      if (data == &button_sensor)
      {
        if (button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
            BUTTON_SENSOR_PRESSED_LEVEL)
        {
          leds_on(LEDS_RED);
        }

        else if (button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) ==
                 BUTTON_SENSOR_RELEASED_LEVEL)
        {
          leds_off(LEDS_RED);
        }
      }
    }

    else if (ev == button_press_duration_exceeded)
    {
      leds_on(LEDS_BLUE);
    }

    else if (ev == serial_line_event_message)
    {
      // A line of input has been received from the serial port
      // Access the received message using the data pointer
      char *message = (char *)data;

      // Compare the received message with predefined commands
      if (strncmp(message, "red.on", strlen("red.on")) == 0)
      {
        leds_on(LEDS_RED);
      }
      else if (strncmp(message, "red.off", strlen("red.off")) == 0)
      {
        leds_off(LEDS_RED);
      }
      else if (strncmp(message, "blue.on", strlen("blue.on")) == 0)
      {
        leds_on(LEDS_BLUE);
      }
      else if (strncmp(message, "blue.off", strlen("blue.off")) == 0)
      {
        leds_off(LEDS_BLUE);
      }
      else if (strncmp(message, "green.on", strlen("green.on")) == 0)
      {
        leds_on(LEDS_GREEN);
      }
      else if (strncmp(message, "green.off", strlen("green.off")) == 0)
      {
        leds_off(LEDS_GREEN);
      }
    }
  }

  PROCESS_END();
}
