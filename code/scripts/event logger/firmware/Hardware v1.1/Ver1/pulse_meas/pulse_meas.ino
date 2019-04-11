unsigned long timestamp;    // This is the timestamp captured from micros() for pulses
unsigned long intTimestamp; // This is the timestamp used for the pin interrupt
unsigned long duration;     // This is the duration of the pulse measured between mouse/photon
bool pinInterrupt = false;  // This flag indicates whether pin interrupt has occurred
bool clkOut = false;        // This controls whether 1/2 the main clock is routed to pin 9
bool clickOut = true;       // This controls whether the click is routed out via pin 9
int pulsePin = 7;           // This is the pin used for capturing the time difference pulse from mouse-to-photon
const byte intPin = 3;      // This is the interrupt pin we will use

const int clickRateHz = 2;        // This is the rate to produce clicks at
const int baseClk = 7812;         // This is the timer clock rate (16MHz / 1024)
const int clickCount = baseClk/clickRateHz-1;   // This is what to count to

// For Arduino Pro Micro (from SparkFun https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro):
//    Pin 3 --> Interrupt 0
//    Pin 2 --> Interrupt 1
//    Pin 0 --> Interrupt 2
//    Pin 1 --> Interrupt 3
//    Pin 7 --> Interrupt 4
// NOTE: This isn't needed if the digitalPinToInterrupt() function works correctly!

void setup() {
  if(clkOut) setClockOutput();      // Setup clock output (conditionally) on pin 9
  else if(clickOut) setClickOutput();  // Setup click output
  
  // Setup the output serial stream
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Begin");

  pinMode(pulsePin, INPUT);         // Setup the pulse measurement pin as an input

  // Setup the interrupt capture info for SW
  pinInterrupt = false;       
  pinMode(intPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(intPin), timestampISR, FALLING);
}

void loop() {
  // Look for positive going pulse here
  duration = pulseIn(pulsePin, HIGH);   // This looks for a pulse (w/ a timeout) most time is spent here
  timestamp = micros();         // Get a timestamp for the event (note this means the pulse timestamp is at the END of the pulse)

  // Do the logging for pin edge events
  if(pinInterrupt){
    // Interrupt pin has toggled, write this to the serial port
    Serial.print(intTimestamp);
    Serial.println(": Pin Toggle"); 
    pinInterrupt = false;                   // Clear the pin interrupt flag
  }

  // Do the logging for the edge detector
  Serial.print(timestamp);                  // Write out the timestamp
  Serial.print(": ");                       // Write out a delimiter
  if(duration > 0) {                        // Check for valid duration (i.e. no timeout)
    Serial.print((float)duration/1000);     // Write out the duration (in ms)
    Serial.println("ms");               
  }
  else {
    Serial.println("Timeout");  // Indicate that timeout occurred
  }
}

void timestampISR(){
  intTimestamp = micros();      // Get a mincrosecond timestamp value (same timebase as pulse timestamp)
  pinInterrupt = true;          // Set the pin interrupt flag and return
}

// This function toggles pin 9 at 1/2 the main clock rate (8MHz)
void setClockOutput(){
  pinMode(9, OUTPUT);                 // Setup pin 9 as OC1A output
  TCCR1A = bit(COM1A0);               // Setup OC1A for output
  TCCR1B = bit(WGM12) | bit(CS10);    // CTC w/ 1/1024 prescale (15.625kHz clock)
  OCR1A = 0;                          // Output every cycle
}

// This function toggles pin 9 at 1/2 the main clock rate (8MHz)
void setClickOutput(){
  pinMode(9, OUTPUT);                             // Setup pin 9 as OC1A output
  TCCR1A = bit(COM1A0);                           // Setup OC1A for output
  TCCR1B = bit(WGM12) | bit(CS12) | bit(CS10);    // CTC w/ 1/1024 prescale (15.625kHz clock)
  OCR1A = clickCount;                             // Output every clickCount cycle to get correct rate
}

