/*============================================================================
  
  ina219.h

  The INA219 "class" provides an interface to the INA219 current/voltage
  monitor, which is widely used in charging circuits for embedded
  Linux appliance. The usual call sequence is

    INA219 *ina219 = ina219_create (...);
    if (ina219_init (ina219, ...))
      {
      if (ina219_get_status (ina219, ... 
      ina219_deinit (ina219);
      }
    ina219_destroy (ina219);

  All voltages are in millivolts, all currents in milliamps,
  and battery capacities in milliamp-hours. These choices allow all
  the calculations to be done using integer arithmetic.

  Note that calculated time to charge or discharge depends on the
  charging (or discharging) current being reasonably constant. In
  practice, this won't be the case. Charging circuits often reduce the
  charging current as the battery approaches full charge. This can make
  the charging time seem to increase as the battery charge increases.
  There's no way around this problem, except to work out a voltage-time
  curve for a specific battery and charger by measurement, and ignore 
  the calculated times completely.

  All methods that return a BOOL return TRUE for success. All methods that
  take a char** set te caller's char* to a descriptive message on failure,
  if the caller initializes the argument to non-null. The caller must free
  this error message eventually.

  Althought most of the "methods" in this "class" return success/failure
  status and error message, it's highly unlikely that _init() will succeed
  and anything else will fail. Even if the wrong device is at the specified
  I2C address, the data acquisition will still produce results -- they will
  just be meaningless results. Really, the only way for _init() to succeed
  and anything else to fail would be to unplug the I2C bus at runtime.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

struct INA219;
typedef struct _INA219 INA219;

// The percentage of the maximum voltage above which we will assume
//  the batteries to be fully charged.
#define INA_FULL_PERCENT 99

// INA219ChargeStatus defines a set of values that can be returned by
//  ina219_get_status, to indicate whether the battery is charging, 
//  discharging, or fully charged. Note that the charge current will probably
//  never get to zero, so we take "fully charged" to indicate that the 
//  battery voltage is above some threshold. Some tweaking may be
//  required, because the "fully charged" voltage quoted by the battery
//  voltage will only be a nominal figure. 
typedef enum _INA219ChargeStatus
  {
  INA219_FULLY_CHARGED = 0,
  INA219_CHARGING = 1,
  INA219_DISCHARGING = 2
  } INA219ChargeStatus;

BEGIN_DECLS

/** Create a INA219 instance, specifying the interface and battery
    properties. Voltages are in millivolts, and capacity is in mA-hours. */ 
INA219     *ina219_create (const char *i2c_dev, int i2c_addr,
               int shunt_milliohms, int battery_voltage_0_percent,
               int battery_voltage_100_percent, int battery_capacity,
               int min_charging_current);

/** Tidy up this INA219 instance. Implicitly calls ina219_uninit(). */
void     ina219_destroy (INA219 *ina219);

/** Initialize this "object". Note that this method can fail, because it
    initializes hardware. */
BOOL     ina219_init (INA219 *self, char **error);

/** Tidy up and free resources. There's no need to call this method if 
    _destroy() is called. _init() and _uninit() can be called repeatedly if
    necessary. Between calls to _init() and _uninit(), the "object" holds a
    reference to an open file descriptor on /dev/i2c-N */
void     ina219_uninit (INA219 *self);

/** Get the "bus voltage", that is, the voltage on pin IN-. The voltage in
    millivolts is set in *mv. The range is 0-32000 mV. This method is 
    called internall by _get_status(); there is no need to call both. */
BOOL     ina219_get_bus_voltage (const INA219 *self, int *mv, char **error);

/** Get the "shunt voltage", that is, the voltage between the IN- and IN+
    pins. In most installations, a +ve shunt voltage indicates that a
    battery is charging, and -ve discharging. The range of values is 
    +/- 320 mV. The charge current can be obtained by dividing this 
    value by the resistance between the IN- and IN+ pins. */
BOOL     ina219_get_shunt_voltage (const INA219 *self, int *mv, char **error);

/** Get the overall status in the various arguments. 
    I hope that the meanings of the arguments is self-explanatory. 
    minutes is the time in minutes to full charge or full discharge, 
    depending on the current direction. */
BOOL     ina219_get_status (const INA219 *self, 
           INA219ChargeStatus *charge_status, 
           int *battery_voltage_mv, int *percent_charged, 
           int *battery_current_mA, int *minutes, char **error);

END_DECLS

