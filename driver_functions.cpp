#include "constants.h"
#include "driver_functions.h"
#include <Arduino.h>

void confTimer1(unsigned int max_counter=15624){

    /// Setup register for timer A - All control register to zero
    TCCR1A = 0; 
    TCCR1B = 0;
    TCCR1C = 0;
    TCNT1  = 0;

    OCR1A = max_counter; /// Assign to the counter register the selected value.

    /// Control register setup
    TCCR1A |= (0 << COM1A1) | (1 << COM1A0); /// Output toggles on match
    TCCR1B |= (1 << WGM12);  /// Set CTC mode of operation.
    TIMSK1 |= (1 << OCIE1A); /// Enables globals interrupts for the timer. 

}

void startTimer1(int prescaler=64){

    uint8_t clock_selection = 0;

    switch(prescaler){
        case 1: 
            clock_selection |= (1 << CS10); /// Prescaler : 1
            break;

        case 8:
            clock_selection |= (1 << CS11); /// Prescaler : 8
            break;

        case 64:
            clock_selection |= (1 << CS11) | (1 << CS10); /// Prescaler : 64
            break;

        case 256:
            clock_selection |= (1 << CS12); /// Prescaler : 256
            break;

        case 1024:
            clock_selection |= (1 << CS12) | (1 << CS10); /// Prescaler : 1024
            break;

        default:
            clock_selection = 0; /// No valid data provided. Clock is disconnected.
    }

    TCCR1B |= clock_selection; 
}


void stopTimer1(){
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); /// Disconnects the clock source.
    TCNT1 = 0; /// Clean the counter.
    digitalWrite(outPin, LOW); /// Reset output pin to default low
}


void serialDecoder(ARDUINO_CONTROLS* ctrl, byte f_byte, byte s_byte, byte t_byte, volatile unsigned int* interrupt_counter){
    /// info variables
    unsigned int packed_controls = 0;

    /// Unpack the CMD first 
    ctrl->command = (CMD)(f_byte >> 6);

    switch(ctrl->command){
        case STEP_CMD:{
            byte to_send[3];
            ctrl->direction = (bool)(f_byte & 0x01);
            ctrl->steps = (int)((s_byte<<8) | (t_byte));
            ctrl->interrupt_to_steps = 2*ctrl->steps -1 ;
            /// Update values to pinout
            ctrl->toStepPins();
            
            /// Check is controller is slept
            if(!ctrl->sleep) digitalWrite(sleepPin, HIGH);
            startTimer1();

            packed_controls = ctrl->packageControls();
            to_send[0] = (packed_controls & 0x00FF);
            to_send[1] = (ctrl->steps & 0xFF00) >> 8;
            to_send[2] = (ctrl->steps) & 0x00FF;

            Serial.write(to_send, 3);
            break;
        }

        case INFO_CMD:{
            byte to_send[6]; 
            /// Respond with the current information for controls
            packed_controls = ctrl->packageControls();
            /// Setup info
            to_send[0] =  (packed_controls & 0xFF00) >>8;
            to_send[1] =  (ctrl->freq_counter & 0xFF000) >> 8;
            to_send[2] = (ctrl->freq_counter) & 0x00FF;

            /// Step info
            to_send[3] = (packed_controls & 0x00FF);
            to_send[4] = (ctrl->steps & 0xFF00) >> 8;
            to_send[5] = (ctrl->steps) & 0x00FF;
            Serial.write(to_send, 6);
            break;

        }
        
        case SETUP_CMD:{
            byte to_send[3];
            ctrl->micro_stepping = (f_byte & 0x38) >> 3;
            ctrl->reset = (f_byte & 0x04) >> 2;
            ctrl->enable = (f_byte & 0x02) >> 1;
            ctrl->sleep = (f_byte & 0x01);
            ctrl->freq_counter = (unsigned int)((s_byte << 8) | (t_byte));
            /// Update values to pinout
            ctrl->toCtrlPins();
            /// Update register
            confTimer1(ctrl->freq_counter);

            packed_controls = ctrl->packageControls();
            to_send[0] = (packed_controls & 0xFF00) >> 8;
            to_send[1] = (ctrl->freq_counter & 0xFF00) >> 8;
            to_send[2] = (ctrl->freq_counter) & 0x00FF;

            Serial.write(to_send, 3);
            

            break;
        }

        case HALT_CMD: {
            stopTimer1();
            if(!ctrl->sleep) digitalWrite(sleepPin, LOW);
            ctrl->steps = 0;
            ctrl->interrupt_to_steps = 0;
            *interrupt_counter = 0;
            // 1 byte response
            byte to_send = 0xC0;
            Serial.write(to_send);
            break;
        }
        
        default:{
            // Not a valid CMD
            Serial.write(0b11100000);
        }
    }
}


bool serialFSM(volatile Serial_States* serial_state, byte* f_byte, byte* s_byte, byte* t_byte){
    switch(*serial_state){
        case SERIAL_IDLE:{
            if(Serial.available()) *serial_state = FIRST_BYTE;
            else *serial_state = SERIAL_IDLE;
            break;
        }

        case FIRST_BYTE:{
            // Save readed value
            *f_byte = Serial.read();
            *serial_state = WAIT_1;
            break;
        }

        case WAIT_1: {
            if(Serial.available()) *serial_state = SECOND_BYTE;
            else *serial_state = WAIT_1;
            break;
        }

        case SECOND_BYTE:{
            *s_byte = Serial.read();
            *serial_state = WAIT_2;
            break;
        }

        case WAIT_2:{
            if(Serial.available()) *serial_state = THIRD_BYTE;
            else *serial_state = WAIT_2;
            break;
        }

        case THIRD_BYTE:{
            *t_byte = Serial.read();
            *serial_state = DONE;
        }

        case DONE: {
            *serial_state = SERIAL_IDLE;
            return true;
        }

        default: *serial_state = SERIAL_IDLE;
    }

    return false;

}