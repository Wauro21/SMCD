#include "constants.h"
#include "driver_functions.h"

/// Serial reception variables
byte f_byte = 0x00; /// First byte
byte s_byte = 0x00; /// Second byte
byte t_byte = 0x00; /// Third byte
bool received_data = false; /// Successful reception flag

/// Step operation variables
volatile unsigned int interruptCounter = 0; /// Number of times the timer interrupt has been rised
volatile Serial_States serial_state = SERIAL_IDLE; /// Current state of Serial FSM
volatile bool limit_halt = false;

/// Operation controls
ARDUINO_CONTROLS controls;


void setup() {

  /// Setup serial communication for control
  Serial.begin(9600);

  /// Pin mode initialization
  pinMode(outPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(highSwitch, INPUT_PULLUP); /// Interrupt mode
  pinMode(lowSwitch, INPUT_PULLUP);  /// Interrupt mode
  pinMode(ms2Pin, OUTPUT); /// M_2 for microstepping
  pinMode(ms1Pin, OUTPUT); /// M_1 for microstepping
  pinMode(ms0Pin, OUTPUT); /// M_0 for microstepping
  pinMode(resetPin, OUTPUT); /// reset pin for driver board
  pinMode(enablePin, OUTPUT); /// enable pin for driver board
  pinMode(sleepPin, OUTPUT); /// sleep pin for driver board
  pinMode(VCC_PIN_A, OUTPUT);
  pinMode(VCC_PIN_B, OUTPUT);


  /// Default values for I/O
  digitalWrite(outPin, LOW);
  digitalWrite(dirPin, LOW);
  digitalWrite(ms2Pin, LOW);
  digitalWrite(ms1Pin, LOW);
  digitalWrite(ms0Pin, LOW);
  digitalWrite(resetPin, HIGH);
  digitalWrite(enablePin, LOW);
  digitalWrite(sleepPin, HIGH);
  digitalWrite(VCC_PIN_A, HIGH);
  digitalWrite(VCC_PIN_B, HIGH);


  /// Attach intterupt to switch pins
  attachInterrupt(digitalPinToInterrupt(highSwitch), limitHalt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(lowSwitch), limitHalt, CHANGE);

  confTimer1(); /// Setup timer configuration
  sei(); /// allow interrupts
}

void loop() {
  if(limit_halt){
    stopTimer1();
    interruptCounter = 0;
    limit_halt = false;
    if(!controls.sleep) digitalWrite(sleepPin, HIGH);
  }

  received_data = serialFSM(&serial_state, &f_byte, &s_byte, &t_byte);
  if(received_data){
    serialDecoder(&controls, f_byte, s_byte, t_byte, &interruptCounter);
    received_data = false;
  }
}


ISR(TIMER1_COMPA_vect){
  if(interruptCounter < controls.interrupt_to_steps) interruptCounter+=1;
  else{
    stopTimer1();
    interruptCounter = 0;
    if(!controls.sleep) digitalWrite(sleepPin, LOW); /// If enabled, send controller to sleep
  } 
    

}

void limitHalt(){
  limit_halt = true;
}