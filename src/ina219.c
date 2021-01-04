/*==========================================================================
  
    ina219.c

    Implementation of the "methods" in ina219.h

    INA219 datasheet is here:
    https://www.ti.com/lit/ds/symlink/ina219.pdf

    Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "defs.h" 
#include "ina219.h" 

// INA219 registers that are used by this implementation.
// See page 18 of the datasheet for others
// Shunt voltage
#define SHUNT_REG 1
// Bus voltage
#define BUS_REG   2

// INA219 structure -- stores all internal data related to this
//  INA219 instance
struct _INA219
  {
  char *i2c_dev; // E.g., /dev/i2c-1
  int i2c_addr;  // E.g., 0x43
  int fd; // For the /dev/i2c-N device
  // The following are battery and system properties passed by the caller.
  int shunt_milliohms; 
  int battery_voltage_0_percent;
  int battery_voltage_100_percent;
  int battery_capacity;
  int min_charging_current;
  };

/*============================================================================

  ina219_register_read_16

  Read a 16-bit register. The protocol (which is similar in most I2C devices)
  is to write a register number (one byte in this case), and read a value
  (16 bits in this case). Note that I've defined the data result as a 
  _signed_ 16-bit value but, by itself, the sign means nothing -- we need
  to look at the datasheet to see how the various bits in the result are
  assigned. I've defined it as signed because the values read by this 
  implementation are, in fact, signed (shunt voltage), or neither
  inherently signed nor unsigned (shunt voltage). Don't read too much
  into this choice -- it just reduces the amount of ugly casting in 
  other parts of the code.

  This method can fail, but it's highly unlikely if _init() suceeded.  

============================================================================*/
static BOOL ina219_register_read_16 (const INA219 *self, BYTE reg, 
       int16_t *data, char **error)
  {
  assert (self != NULL);
  assert (self->fd >= 0); // Don't allow this to be called before _init()
  BOOL ret = FALSE;
  BYTE buff[2];
  buff[0] = reg;
  // Write the register number to the bus
  if (write (self->fd, &buff, 1))
    {
    // Then read the two-byte result
    if (read (self->fd, buff, 2) == 2)
      {      
      *data = (buff[0] << 8 ) | buff[1];
      ret = TRUE;
      }
    else
      {
      if (error) asprintf (error, "Failed to read I2C device: %s\n", 
        strerror (errno));
      }
    }
  else
    {
    if (error) asprintf (error, "Failed to write I2C device: %s\n", 
      strerror (errno));
    }
  return ret;
  } 

/*============================================================================

  ina219_get_bus_voltage

============================================================================*/
BOOL ina219_get_bus_voltage (const INA219 *self, int *mv, char **error)
  {
  BOOL ret = FALSE;
  int16_t regval;
  if (ina219_register_read_16 (self, BUS_REG, (int16_t*) &regval, error))
    {
    // This arcane-looking math follows from the fact that the bus voltage
    //  is in units of 4mV, but shifted up three bits. The bottom three
    //  bits have other meanings, not related to the voltage. We mask
    //  off the bottom three bits, to get the voltage as a multiple of 8
    //  mV (because of the original three-bit shift), then shift down one
    //  bit to get the final result in mV. 
    // See page 23 of the datasheet
    *mv = (regval & 0xFFF8 ) >> 1;
    ret = TRUE;
    }

  return ret;
  }

/*============================================================================

  ina219_get_shunt_voltage

============================================================================*/
BOOL ina219_get_shunt_voltage (const INA219 *self, int *mv, char **error)
  {
  BOOL ret = FALSE;
  int16_t regval;
  if (ina219_register_read_16 (self, SHUNT_REG, (int16_t*) &regval, error))
    {
    // The shunt register reads units of 10uV. To get mV we must divide
    //   by 100 (that is, multiply by 10 and divide by 1000)
    // See page 20 of the datasheet
    *mv = regval / 100;
    ret = TRUE;
    }

  return ret;
  }

/*============================================================================

  ina219_create

============================================================================*/
INA219 *ina219_create (const char *i2c_dev, int i2c_addr,
          int shunt_milliohms, int battery_voltage_0_percent,
          int battery_voltage_100_percent, int battery_capacity,
          int min_charging_current)
  {
  INA219 *self = malloc (sizeof (INA219));
  memset (self, 0, sizeof (INA219));
  self->i2c_dev = strdup (i2c_dev);
  self->i2c_addr = i2c_addr;
  self->fd = -1;
  self->shunt_milliohms = shunt_milliohms;
  self->battery_voltage_0_percent = battery_voltage_0_percent;
  self->battery_voltage_100_percent = battery_voltage_100_percent;
  self->battery_capacity = battery_capacity;
  self->min_charging_current = min_charging_current;
  return self;
  }

/*============================================================================
  ina219_destroy
============================================================================*/
void ina219_destroy (INA219 *self)
  {
  if (self)
    {
    ina219_uninit (self);
    if (self->i2c_dev) free (self->i2c_dev);
    free (self);
    }
  }

/*============================================================================

  ina219_init

  Store the file descriptor for the /dev/i2c-N device, and use an 
  ioctl() to set the bus slave address.

============================================================================*/
BOOL ina219_init (INA219 *self, char **error)
  {
  assert (self != NULL);
  error = error;
  BOOL ret = FALSE;
  self->fd = open (self->i2c_dev, O_RDWR);
  if (self->fd >= 0)
    {
    // Set the I2C slave address that was supplied when this
    //   object was created
    if (ioctl (self->fd, I2C_SLAVE, self->i2c_addr) >= 0)
      {
      ret = TRUE;
      }
    else
      {
      asprintf (error, "Can't intialize I2C device: %s", strerror (errno));
      }
    }
  else
    {
    asprintf (error, "Can't open I2C device: %s", strerror (errno));
    }
  return ret;
  }

/*============================================================================
  ina219_uninit
============================================================================*/
void ina219_uninit (INA219 *self)
  {
  assert (self != NULL);
  if (self->fd >= 0) close (self->fd);
  self->fd = -1;
  }

/*============================================================================

  ina219_get_status

  Work out the overall charge status, using the bus voltage, shunt voltage,
  and the properties of the battery.

============================================================================*/
BOOL ina219_get_status (const INA219 *self, INA219ChargeStatus *charge_status, 
      int *battery_voltage_mv, int *percent_charged, 
      int *battery_current_mA, int *minutes, char **error)
  {
  BOOL ret = FALSE;
  int mv = 0;
  if (ina219_get_bus_voltage (self, &mv, error))
    {
    *battery_voltage_mv = mv;

    // Work out the percentage charge. The user has specified the full-charge
    //  and no-charge voltages, and the battery voltage is assumed to lie
    //  in this range. If the voltage is half-way between no-charge and
    //  full-charge, we take the charge level to be 50%. The no-charge
    //  voltage won't be zero, because the powered device will have stopped
    //  working long before that point is reached. However, we limit the
    //  reported charge to "0%", just in case of odd circumstances.  

    // There is, of course, no way to measure the charge status of most 
    //  batteries _except_ in terms of voltage.

    *percent_charged = 100 * (mv - self->battery_voltage_0_percent) / 
        (self->battery_voltage_100_percent - self->battery_voltage_0_percent);
    if (*percent_charged > 100) *percent_charged = 100;
    if (*percent_charged < 0) *percent_charged = 0;

    if (ina219_get_shunt_voltage (self, &mv, error))
      {
      ret = TRUE;

      // Calculate the battery current as shunt voltage divided by shunt
      //  resistance. Note that working in milli-units allows us to do
      //  all the following math in integers.
      int mA = mv * 1000 / self->shunt_milliohms;
      *battery_current_mA = mA;

      // Don't try work out whether the battery is charging or discharging
      //  if the voltage is very close to the maximum. In practice, the
      //  voltage will oscillate around the maximum value, and the current
      //  will reverse direction. There's no point reporting that.  
      if (*percent_charged >= INA_FULL_PERCENT || 
             mA < self->min_charging_current)
        {
        *charge_status = INA219_FULLY_CHARGED;
        }
      else
        {
        // Note that the INA219 is normally connecting in such a way that
        //  a positive current means the battery is charging. However, it's
        //  not inevitable, and it may be necessary to reverse the logic
        //  below.
        if (mA > 0)
          *charge_status = INA219_CHARGING;
        else
          *charge_status = INA219_DISCHARGING;
        }

      if (*charge_status == INA219_FULLY_CHARGED)
        *minutes = 0;
      else
        {
        // There's really now way to work out the time to full charge
        //  or discharge, just based on the voltage and current. Neither 
        //  batteries nor charging circuits behave linearly enough. A
        //  serious effort to report these times will have to be based on
        //  characterizing the voltage/time relationship for a specific
        //  battery and charger. Here we do the (poor) best we can, given
        //  the limited information available.

        if (mA >= 0)
          {
          int remaining_capacity = (100 - *percent_charged) * 
                self->battery_capacity / 100; 
	  int sec = 3600 * remaining_capacity / (double) mA; 
          *minutes = sec / 60;
          }
        else
          {
          int remaining_capacity = *percent_charged * 
                self->battery_capacity / 100; 
	  int sec = 3600 * remaining_capacity / (double) -mA; 
          *minutes = sec / 60;
          }
	}
      }
    }

  return ret;
  }


