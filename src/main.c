/*============================================================================
  
    main.c

    A test driver for the INA219 "class". This program uses the INA219 
    class to collect current and voltage data using I2C. It's been tested
    on a charging system that uses to 18650 batteries in series, with 
    a 0.1 ohm shunt resistor for measuring the battery current draw. 

    Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "defs.h" 
#include "ina219.h" 

// Set the I2C address of the INA219. This will be in the range
//  0x40-0x4F, depending on how the address setting pins (7 and 8)
//  are connected 
#define I2C_ADDR 0x42

// Set the I2C device. On a Raspberry Pi, the I2C on the GPIO header
//  is device 1, not 0.
#define I2C_DEV "/dev/i2c-1"

// An 18650 battery has a nominal voltage of 3.7V. In practice, when 
//  fully charged mine measure 4.2V. When they get below about 3V, they
//  lose the ability to provide a 5V supply through a voltage regulator.
// The manufacturer's claimed capacity is 2400 mA.hr (which does not
//  increase, of course, when there are two in series.)

// Voltages in mV
#define BATTERY_VOLTAGE_100_PERCENT 8260
#define BATTERY_VOLTAGE_0_PERCENT 6000 
// Capacity in mA.hr
#define BATTERY_CAPACITY 2400

// Minimum charging current in mA
#define MIN_CHARGING_CURRENT 10

// Shunt resistance in mOhms -- this is a function of the circuit,
//  not the battery. A value of 0.1 ohms is pretty typical in INA219-based
//  designs.
#define SHUNT_MILLIOHMS 100

/*============================================================================

  main

============================================================================*/
int main (int argc, char **argv)
  {
  argc = argc; argv = argv; // Suppress warnings

  // Create the INA219 object, passing the I2C settings, and shunt
  //  resistance, and the battery properties. Note that this 
  //  call only stores values, and will always succeed
  INA219 *ina219 = ina219_create (I2C_DEV, I2C_ADDR, SHUNT_MILLIOHMS,
                     BATTERY_VOLTAGE_0_PERCENT, BATTERY_VOLTAGE_100_PERCENT,
                     BATTERY_CAPACITY, MIN_CHARGING_CURRENT);

  // Initialse the INA219 "class". If this fails, *error will be initialized
  //   to an error message
  char *error = NULL;
  if (ina219_init (ina219, &error))
    {
    // Get and print the charging status. Note that a -ve value for
    //   the battery current indicates that the battery is discharging.
    INA219ChargeStatus charge_status; // See ina219.h for values of this enum.
    int mV;
    int percent_charged;
    int battery_current_mA;
    int minutes;
    if (ina219_get_status (ina219, &charge_status, &mV, &percent_charged,
           &battery_current_mA, &minutes,
           &error))
      {
      switch (charge_status)
        {
        case INA219_FULLY_CHARGED:
          printf ("Fully charged\n");
          break;
        case INA219_CHARGING:
          printf ("Charging, %d minutes until fully charged\n", minutes);
          break;
        case INA219_DISCHARGING:
          printf ("Discharging, %d minutes left\n", minutes);
          break;
        }
      printf ("Battery voltage: %.2f V\n", mV / 1000.0); // Convert to V
      printf ("Battery current: %d mA\n", battery_current_mA); 
      printf ("Battery charge: %.d %%\n", percent_charged); 
      }
    else
      {
      fprintf (stderr, "%s: %s\n", argv[0], error);
      }
    }
  else
    {
    fprintf (stderr, "Can't set up INA219: %s\n", error);
    free (error); 
    }
  }

