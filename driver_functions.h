#ifndef DRIVER_FUNCTIONS_H
#define DRIVER_FUNCTIONS_H

#include <Arduino.h>


/// @brief confTimer1 setups the 16-bit timer 1 register of the arduino Uno.
/// Operation is set to CTC and enables interrupt masking of the counter.
/// This only setups mode of operation, clock selection is left unconnected. 
/// @param max_counter corresponds to the max number that the counter can
/// reach before starts over. It's related with the desired frequency 
/// according to the equation : f_out = f_clk / (2 * N * (1 + max_counter))
void confTimer1(unsigned int max_counter=15624);


/// @brief Starts the timer 1 by connecting the clock source, with its corresponding
/// prescaler, to the counter logic. 
/// @param prescaler prescaler for the clock source. valid values are: 1, 8, 64, 256, 1024.
void startTimer1(int prescaler=64);

/// @brief Stops the timer 1 (and by extension the output), by disconecting the clock source.
void stopTimer1();

/// @brief FSM that handles the recieved bytes from host. For each command, three bytes are saved.
/// When 3 bytes are received the function return true.
/// @param serial_state Corresponds to the variable that holds the current state of the FSM
/// @param f_byte Variable to hold the first received byte.
/// @param s_byte Variable to hold the second received byte.
/// @param t_byte Variable to hold the third received byte.
/// @return true if three bytes are correctly received.
bool serialFSM(volatile Serial_States* serial_state, byte* f_byte, byte* s_byte, byte* t_byte);

/// @brief Decodes the three received bytes from the host onto the corresponding commands for the driver.
/// The function manages all the changes 
/// @param ctrl Control parameters of the driver
/// @param f_byte Variable to hold the first received byte.
/// @param s_byte Variable to hold the second received byte.
/// @param t_byte Variable to hold the third received byte.
/// @param interrupt_counter Counter that holds the timer1 a interrupt count
void serialDecoder(ARDUINO_CONTROLS* ctrl, byte f_byte, byte s_byte, byte t_byte, volatile unsigned int* interrupt_counter);

#endif