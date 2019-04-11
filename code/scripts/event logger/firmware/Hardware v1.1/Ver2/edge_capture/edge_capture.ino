// Defines for event types
#define     M1      0       // Index for mouse 1 (left) button timestamp/flag
#define     M2      1       // Index for mouse 2 (right) button timestamp/flag
#define     PD      2       // Index for photodiode input timestamp/flag
#define     SW      3       // Infex for software (digital) input timestamp/flag
#define     INTS    4       // Total number of interrupts

// Parameters for autoclicking
#define MOUSE_DOWN_MS   100   // Time for mouse down (in autoclick)
#define MOUSE_UP_MS     900   // Time for mouse up (in autoclick)

#define MAX_CMD_LEN     4     // This is the max command length (for now just 'on\n' or 'off\n' for autoclick)

#define BAUD_RATE       115200      // This is the UART baud rate

// Storage for timestamps and flags as well as name string lookup
char* names[INTS] = {"M1", "M2", "PD", "SW"};      // Names for reporting
unsigned long timestamps[INTS] = {0};      // These are the timestamps captured from micros() for pulses
bool flags[INTS] = {false};                // These are the flags set when a timestamp is captured

// Pin setup (from Arduino's pespective)
const byte m1Pin = 2;           // Mouse left button input on D7
const byte m2Pin = 1;           // Mouse right button input on D1 
const byte pdPin = 0;           // Photosensor input on D0
const byte swPin = 3;           // Software input on D3
const byte clickPin = 9;        // Pin to use for clicking
const byte ledPin = 15;         // LED pin used to indicate status
const byte analogPin = 0;       // Analog pin used for PD input

// Clock or click output control on Pin 9 (BOTH CANNOT BE SET AT ONCE!)
bool clkOut = false;        // This controls whether 1/2 the main clock is routed to pin 9
bool clickOut = false;       // This controls whether the click is routed out via pin 9

// Management flags for autoclicking
bool mousePressed = false;  // Flag to store current mouse state
int mouseCount = 0;         // Count to use for mouse

// String storage and count for input
char cmd[MAX_CMD_LEN] = {0};    // Command storage buffer
char cmdIdx = 0;                // Command storage index

// For Arduino Pro Micro (from SparkFun https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro):
//    Pin 3 --> Interrupt 0
//    Pin 2 --> Interrupt 1
//    Pin 0 --> Interrupt 2
//    Pin 1 --> Interrupt 3
//    Pin 7 --> Interrupt 4
// NOTE: This isn't needed if the digitalPinToInterrupt() function works correctly!

// Do this once (setup click output, LED, serial port, and interrupts)
void setup() {
    setClickOutput();                       // Setup click output
    if(clkOut) setClockOutput();            // Setup clock output (conditionally) on pin 9

    // Setup the LED, blink the turn off
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    delay(300);
    digitalWrite(ledPin, HIGH);
  
     // Setup the output serial stream
    Serial.begin(BAUD_RATE);
    while(!Serial);
    Serial.println("Hardware Event Logger (FW Rev 2.3)");
    
    // Setup the interrupt pins
    pinMode(m1Pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(m1Pin), m1ISR, FALLING);
    pinMode(m2Pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(m2Pin), m2ISR, FALLING);
    pinMode(pdPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(pdPin), pdISR, FALLING);
    pinMode(swPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(swPin), swISR, FALLING);
}

// Do this repeated (check for events and write to serial port)
void loop() {
    int i = 0;
    char c;
    int val;
    unsigned long t;

    // Do the PD analog read/reporting here
    val = analogRead(analogPin);
    t = micros();
    Serial.print(t);
    Serial.print(":");
    Serial.println(val);

    // Check for interrupts and report their times if they occurred
    for(i = 0; i < INTS; i++){
        if(flags[i]){
            Serial.print(timestamps[i]); 
            Serial.print(":");
            Serial.println(names[i]);
            flags[i] = false;
        }
    }
    // Check for new serial data
    if(Serial.available() > 0){
        c = Serial.read();
        // Check for max length or out of range
        if(c == '\n' || cmdIdx >= MAX_CMD_LEN) processCmd(); // Process the command
        else cmd[cmdIdx++] = c;     // Add the character to the buffer
    }
}

// ISRs are here
void m1ISR(){
    timestamps[M1] = micros();
    flags[M1] = true;
}
void m2ISR(){
    timestamps[M2] = micros();
    flags[M2] = true;
}
void pdISR(){
    timestamps[PD] = micros();
    flags[PD] = true;
}
void swISR(){
    timestamps[SW] = micros();
    flags[SW] = true;
}
// Timer interrupt handler (roughly ms tick)
SIGNAL(TIMER0_COMPA_vect){
    if(!clickOut) digitalWrite(clickPin, LOW);     // Get rid of the click for good
    else {
        mouseCount++;         // Increment the mouse count
        if(mousePressed && mouseCount >= MOUSE_DOWN_MS){
            mousePressed = false;           // Clear flag
            mouseCount = 0;                 // Clear count
            digitalWrite(clickPin, LOW);    // Set the mouse up
        }
        else if(!mousePressed && mouseCount >= MOUSE_UP_MS){
            mousePressed = true;
            mouseCount = 0;
            digitalWrite(clickPin, HIGH);   // Set the mouse down and report click (don't use ISR here)
            flags[M1] = true;
            timestamps[M1] = micros();
        }
    }
}

// This function toggles pin 9 at 1/2 the main clock rate (8MHz)
void setClockOutput(){
  pinMode(clickPin, OUTPUT);                 // Setup pin 9 as OC1A output
  TCCR1A = bit(COM1A0);               // Setup OC1A for output
  TCCR1B = bit(WGM12) | bit(CS10);    // CTC w/ 1/1024 prescale (15.625kHz clock)
  OCR1A = 0;                          // Output every cycle
}

// This function toggles pin 9 at the divider set by clickCount
void setClickOutput(){
  pinMode(clickPin, OUTPUT);                              // Setup pin 9 as OC1A output
  OCR0A = 0xAF;
  TIMSK0 |= bit(OCIE0A);
}

// Simple routine to process an 'on' or 'off' command character by character
void processCmd(){
    cmdIdx = 0;                             // Reset the command index
    // For now just decide whether we got "on\n" or "off\n", only change on these values
    if(cmd[0] != 'o') return;               // If first character is not 'o' then return
    if(cmd[1] == 'n') {                     // 2nd character is 'n' (we have the full "on")
        clickOut = true;                    // Set clickOut flag true
        detachInterrupt(digitalPinToInterrupt(m1Pin));                  // Turn off M1 interrupt while we auto-click
        digitalWrite(ledPin, LOW);          // Turn on LED
    }
    else if(cmd[1] != 'f' || cmd[2] != 'f') // 2nd or 3rd character is not 'f' (we don't know what this is)
        return;
    else {                                  // 3rd character is 'f' (we have the full "off")
        clickOut = false;                   // Set clickOut flag false
        attachInterrupt(digitalPinToInterrupt(m1Pin), m1ISR, FALLING);  // Re-attach interrupt for M1
        digitalWrite(ledPin, HIGH);         // Turn off LED                   
    } 
}

