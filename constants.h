#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

/// @brief Pin I/O definitions
const byte outPin = 9; /// Step pin to the stepper controller.
const byte dirPin = 6; /// Direction pin to the stepper controller.
const byte highSwitch = 2; /// High limit switch pin.
const byte lowSwitch = 3; /// Low limit switch pin.
const byte ms2Pin = 11; /// Microstepping mode pin 2
const byte ms1Pin = 12; /// Microstepping mode pin 1
const byte ms0Pin = 13; /// Microstepping mode pin 0
const byte resetPin = 8; /// Reset pin for driver board. Logic low for reset.
const byte enablePin = 10; /// Enable pin for driver board. Logic low for enable. 
const byte sleepPin = 7; /// Sleep pin for driver board. Logic low for sleep.
const byte VCC_PIN_A = 4; /// Pin used to run vcc rail for limit switch
const byte VCC_PIN_B = 5; /// Pin used to run vcc rail for limit switch

/// @brief States for the FSM that handles the serial commands received. 
enum Serial_States {
    SERIAL_IDLE, 
    FIRST_BYTE,
    WAIT_1, 
    SECOND_BYTE,
    WAIT_2,
    THIRD_BYTE,
    DONE, 
    HALT
};

/// @brief Possible values for the bits[7:6] of the arriving control packet. 2 unused
/// states are available for coding (01-10).
enum CMD {
    STEP_CMD=0x00,
    INFO_CMD=0x01,
    SETUP_CMD=0x02,
    HALT_CMD = 0x03,
};

/// @brief A structure that groups all control parameters of the driver.
struct ARDUINO_CONTROLS{
    CMD command;
    byte micro_stepping;
    bool reset; /// Reset for the driver board is logic low.
    bool enable; /// Enable logic low 
    bool sleep; /// Sleep logic low. This disables holding torque.
    bool direction; /// false -> Counterclockwise | true -> Clockwise
    bool halt; /// Flag that halts motor operation
    unsigned int steps; /// Number of steps to take
    unsigned int interrupt_to_steps; /// Number of interrupts of the timer to achieve the desired steps | interrupt_to_steps = 2*steps -1 
    unsigned int freq_counter; /// Counter for the desired frequency

    /// @brief Constructor for the structure. Defaults the CMD to the INFO_CMD, that sends a packet with the current state of the driver.
    /// micro_stepping to full step. Disables reset and sleep modes. Enables the controller board. Sets the direction to clockwise. 
    /// halt to false. The number of steps and interrupts needed to zero and the frequency counter to zero. Sends the information to the pinout.
    ARDUINO_CONTROLS(){
        command = INFO_CMD;
        micro_stepping = 0x00;/// Full step by default
        reset = true;
        enable = false; /// Controller is connected by default
        sleep = true; /// This allows for default holding torque
        direction = false;
        halt = false;
        steps = 0;
        interrupt_to_steps = 0;
        freq_counter = 0;
        this->toCtrlPins();
    };

    /// @brief Writes the current setup data of the structure to the pinout.
    void toCtrlPins(){
        byte ms2 = ((this->micro_stepping) >> 2) & 0x01;
        byte ms1 = ((this->micro_stepping) >> 1) & 0x01;
        byte ms0 = ((this->micro_stepping) & 0x01);
        /// Apply to pinout
        digitalWrite(ms2Pin, ms2); /// Pin 2 of microstepping controls
        digitalWrite(ms1Pin, ms1); /// Pin 1 of microstepping controls
        digitalWrite(ms0Pin, ms0); /// Pin 0 of microstepping controls
        digitalWrite(resetPin, this->reset); 
        digitalWrite(enablePin, this->enable);
        digitalWrite(sleepPin, this->sleep);
    }

    /// @brief Writes the current step data to the pinout.
    void toStepPins(){
        digitalWrite(dirPin, this->direction);        
    }

    /// @brief Packages the information on the structure to be sent via serial.
    /// @return 16-bit array holding in the MSB the Setup data and in the LSB the step data.
    unsigned int packageControls(){
        byte to_send_a = 0x00;
        byte to_send_b = 0x00;
        to_send_a |= (0x02 << 6) | (this->micro_stepping << 3) | (this->reset << 2) | (this-> enable << 1) | (this->sleep);
        to_send_b |= this->direction;
        return ((to_send_a << 8) | (to_send_b));
    }

};

#endif