// Defines for event types
#define     M1      0       // Index for mouse 1 (left) button timestamp/flag
#define     M2      1       // Index for mouse 2 (right) button timestamp/flag
#define     PD      2       // Index for photodiode input timestamp/flag
#define     SW      3       // Infex for software (digital) input timestamp/flag
#define     INTS    4       // Total number of interrupts

// Parameters for autoclicking
#define MAX_MOUSE_DOWN_MS   1000   // Time for mouse down (in autoclick)

#define MAX_CMD_LEN     8           // This is the max command length (for now just 'on\n' or 'off\n' for autoclick)
#define BAUD_RATE       115200      // This is the UART baud rate

// Storage for timestamps and flags as well as name string lookup
char* vString = "Hardware Event Logger v1.2 (FW Rev 3.0)";      // Version print string
char* names[INTS] = {"M1", "M2", "PD", "SW"};      // Names for reporting
unsigned long timestamps[INTS] = {0};      // These are the timestamps captured from micros() for pulses
bool flags[INTS] = {false};                // These are the flags set when a timestamp is captured
bool reportAdc = true;                     // Controls whether ADC value is reported

// Pin setup (from Arduino's pespective)
const byte m1Pin = 2;           // Mouse left button input on D7
const byte m2Pin = 1;           // Mouse right button input on D1 
const byte pdPin = 0;           // Photosensor input on D0
const byte swPin = 3;           // Software input on D3
const byte clickPin = 10;        // Pin to use for clicking
const byte analogPin = 0;       // Analog pin used for PD input

const byte ledR = 15;
const byte ledG = 14;
const byte ledB = 16;

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

// This function toggles pin 9 at the divider set by clickCount
void setClickOutput(){
  pinMode(clickPin, OUTPUT);                              // Setup pin 9 as OC1A output
  OCR0A = 0xAF;
  TIMSK0 |= bit(OCIE0A);
}

void mouseDown(){
    clickOut = true;
    detachInterrupt(digitalPinToInterrupt(m1Pin));      // Turn off M1 interrupt while we auto-click
    digitalWrite(clickPin, HIGH);                       // Set the mouse down (set mouse up in timer ISR above)
    timestamps[M1] = micros();                          // Report mouse down
    flags[M1] = true;
    digitalWrite(ledB, LOW);                            // Turn on the blue LED
}

void mouseUp(){
    clickOut = false;
    digitalWrite(ledB, HIGH);         // Turn off LED
    digitalWrite(clickPin, LOW);      // Set the mouse up                   
    attachInterrupt(digitalPinToInterrupt(m1Pin), m1ISR, FALLING);  // Re-attach interrupt for M1
}

// Simple routine to process an 'on' or 'off' command character by character
void processCmd(){
    cmdIdx = 0;                             // Reset the command index
    // String parsing for various commands
    if(cmd[0] == 'i') {                     // Check for info command
        Serial.println(vString);
        return;
    }
    if(cmd[1] != 'o') return;               // If first character isn't 'o' we aren't interested
    if(cmd[2] == 'n') {                     // 2nd character is 'n' (we have the full "on")
        if(cmd[0] == 'a') {
            reportAdc = true;
            digitalWrite(ledR, HIGH);
        }
        else if(cmd[0] == 'c') mouseDown();
    }
    else if(cmd[2] != 'f' || cmd[3] != 'f') // 2nd or 3rd character is not 'f' (we don't know what this is)
        return;
    else {                                  // 3rd character is 'f' (we have the full "off")
        if(cmd[0] == 'a') {
            reportAdc = false;
            digitalWrite(ledR, LOW);
        }
        else if(cmd[0] == 'c') mouseUp();
    } 
}

// Do this once (setup click output, LED, serial port, and interrupts)
void setup() {
    setClickOutput();                       // Setup click output
    //if(clkOut) setClockOutput();            // Setup clock output (conditionally) on pin 9

    // Setup the LED, blink the turn off
    pinMode(ledR, OUTPUT);
    pinMode(ledG, OUTPUT);
    pinMode(ledB, OUTPUT);

    digitalWrite(ledB, HIGH);
    digitalWrite(ledR, HIGH);

    // Blink green LED
    digitalWrite(ledG, LOW);
    delay(300);
    digitalWrite(ledG, HIGH);
  
     // Setup the output serial stream
    Serial.begin(BAUD_RATE);
    while(!Serial);
    Serial.println(vString);
    
    // Setup the interrupt pins
    pinMode(m1Pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(m1Pin), m1ISR, FALLING);
    pinMode(m2Pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(m2Pin), m2ISR, FALLING);
    pinMode(pdPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(pdPin), pdISR, RISING);
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
    if(reportAdc){
        val = analogRead(analogPin);
        t = micros();
        Serial.print(t);
        Serial.print(":");
        Serial.println(val);
    }

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
    if(!clickOut) mouseUp();     // Get rid of the click for good
    else {
        mouseCount++;         // Increment the mouse count
        if(mouseCount >= MAX_MOUSE_DOWN_MS){
            clickOut = false;               // Clear flag
            mouseCount = 0;                 // Clear count
            mouseUp();
        }
    }
}

//// This function toggles pin 9 at 1/2 the main clock rate (8MHz)
//void setClockOutput(){
//  pinMode(clickPin, OUTPUT);                 // Setup pin 9 as OC1A output
//  TCCR1A = bit(COM1A0);               // Setup OC1A for output
//  TCCR1B = bit(WGM12) | bit(CS10);    // CTC w/ 1/1024 prescale (15.625kHz clock)
//  OCR1A = 0;                          // Output every cycle
//}
