#define DEBUG

#define MIDI_PC_CMD_ID 0xC0
#define MIDI_CHANNEL 0x02
#define MIDI_PC_CMD (MIDI_PC_CMD_ID + MIDI_CHANNEL)

#define PIN_RGB_LED_RED 6
#define PIN_RGB_LED_GREEN 7
#define PIN_RGB_LED_BLUE 8
#define PIN_STATUS_LED 9
#define PIN_FSW_LEFT_OUTER 5
#define PIN_FSW_LEFT_INNER 4
#define PIN_FSW_RIGHT_INNER 3
#define PIN_FSW_RIGHT_OUTER 2
#define PIN_LED_LEFT_OUTER 13
#define PIN_LED_LEFT_INNER 12
#define PIN_LED_RIGHT_INNER 11
#define PIN_LED_RIGHT_OUTER 10

#define NUM_BUTTONS 4
#define PRESETS_PER_BANK NUM_BUTTONS
#define BANK_MIN 0
#define BANK_MAX 2
#define PRESET_MIN 0
#define PRESET_MAX 11

uint8_t currentBank = 0x00;
uint8_t currentPreset = 0x00;
bool hasPendingActions = false;

const unsigned long multipressDebounceDelay = 20;
const unsigned long debounceDelay = 40;

typedef struct Button {
    unsigned long timeMostRecentlyEqual;
    unsigned long timeWhenPendingStarted;
    bool actionPending;
    bool isPressed;
    uint8_t pin;
    uint8_t ledPin;
    uint8_t lastState;
} Button;

Button buttons[NUM_BUTTONS] = {
    { .timeMostRecentlyEqual=0xffffffff, .timeWhenPendingStarted=0x00000000, .actionPending=false, .isPressed=false, .pin=PIN_FSW_LEFT_OUTER, .ledPin=PIN_LED_LEFT_OUTER, .lastState=HIGH },
    { .timeMostRecentlyEqual=0xffffffff, .timeWhenPendingStarted=0x00000000, .actionPending=false, .isPressed=false, .pin=PIN_FSW_LEFT_INNER, .ledPin=PIN_LED_LEFT_INNER, .lastState=HIGH },
    { .timeMostRecentlyEqual=0xffffffff, .timeWhenPendingStarted=0x00000000, .actionPending=false, .isPressed=false, .pin=PIN_FSW_RIGHT_INNER, .ledPin=PIN_LED_RIGHT_INNER, .lastState=HIGH },
    { .timeMostRecentlyEqual=0xffffffff, .timeWhenPendingStarted=0x00000000, .actionPending=false, .isPressed=false, .pin=PIN_FSW_RIGHT_OUTER, .ledPin=PIN_LED_RIGHT_OUTER, .lastState=HIGH }
};

void SetRgbLed(uint8_t r, uint8_t g, uint8_t b) {
    digitalWrite(PIN_RGB_LED_RED, r);
    digitalWrite(PIN_RGB_LED_GREEN, g);
    digitalWrite(PIN_RGB_LED_BLUE, b);
}

void UpdateLeds(uint8_t bank, uint8_t preset) {
    switch (bank) {
        case 0: SetRgbLed(HIGH, LOW, LOW); break;
        case 1: SetRgbLed(LOW, HIGH, LOW); break;
        case 2: SetRgbLed(LOW, LOW, HIGH); break;
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        digitalWrite(buttons[i].ledPin, LOW);
    }
    short pbDiff = preset - (bank * 4);
    if (pbDiff < PRESETS_PER_BANK || pbDiff >= 0) {
        digitalWrite(buttons[pbDiff].ledPin, HIGH);
    }
}

void SendMidiPc(uint8_t program_number) {
    Serial.write(MIDI_PC_CMD);
    Serial.write(program_number);
}

void setup() {

    // Serial.begin(115200);
    // Set MIDI baud rate:
    Serial.begin(31250);
    
    pinMode(PIN_RGB_LED_RED, OUTPUT);
    pinMode(PIN_RGB_LED_GREEN, OUTPUT);
    pinMode(PIN_RGB_LED_BLUE, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);

    digitalWrite(PIN_RGB_LED_RED, LOW);
    digitalWrite(PIN_RGB_LED_GREEN, LOW);
    digitalWrite(PIN_RGB_LED_BLUE, LOW);
    digitalWrite(PIN_STATUS_LED, LOW);

    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        pinMode(buttons[i].ledPin, OUTPUT);
        digitalWrite(buttons[i].ledPin, HIGH);
        delay(200);
        digitalWrite(buttons[i].ledPin, LOW);
    }
    
    // status led on for 3 seconds to indicate termination of power-on sequence
    delay(200);
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(3000);
    digitalWrite(PIN_STATUS_LED, LOW);

    // startup at preset 0, bank 0
    currentBank = 0;
    currentPreset = 0;
    digitalWrite(PIN_RGB_LED_RED, HIGH);
    digitalWrite(buttons[0].ledPin, HIGH);
}

void loop() {
    
    uint8_t reading;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        reading = digitalRead(buttons[i].pin);
        if (reading == buttons[i].lastState)
            buttons[i].timeMostRecentlyEqual = millis();
        else if (millis() - buttons[i].timeMostRecentlyEqual > debounceDelay) {
            buttons[i].lastState = reading;
            if (reading == LOW) {
                buttons[i].timeWhenPendingStarted = millis();
                buttons[i].actionPending = true;
            }
            // Serial.print(i);
            // Serial.print(" changed to ");
            // Serial.println(buttons[i].lastState);
        }
    }
    // both left buttons pressed
    if (buttons[0].actionPending
        && buttons[1].actionPending
        && (millis() - buttons[0].timeWhenPendingStarted > multipressDebounceDelay
            || millis() - buttons[1].timeWhenPendingStarted > multipressDebounceDelay)) {

        // Serial.println("bank down");
        
        // bank down
        if (currentBank == 0)
            currentBank = 2;
        else
            currentBank--;
        UpdateLeds(currentBank, currentPreset);
        SendMidiPc(currentPreset);

        buttons[0].actionPending = false;
        buttons[1].actionPending = false;
        buttons[0].timeWhenPendingStarted = 0xffffffff;
        buttons[1].timeWhenPendingStarted = 0xffffffff;
    }
    // both right buttons pressed
    if (buttons[2].actionPending
        && buttons[3].actionPending
        && (millis() - buttons[2].timeWhenPendingStarted > multipressDebounceDelay
            || millis() - buttons[3].timeWhenPendingStarted > multipressDebounceDelay)) {

        // Serial.println("bank up");

        // bank up
        if (currentBank == 2)
            currentBank = 0;
        else
            currentBank++;
        UpdateLeds(currentBank, currentPreset);
        SendMidiPc(currentPreset);
        
        buttons[2].actionPending = false;
        buttons[3].actionPending = false;
        buttons[2].timeWhenPendingStarted = 0xffffffff;
        buttons[3].timeWhenPendingStarted = 0xffffffff;
    }
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (buttons[i].actionPending && millis() - buttons[i].timeWhenPendingStarted > multipressDebounceDelay) {
            
            // Serial.print("preset ");
            // Serial.println(i);
            
            // change preset
            currentPreset = currentBank * PRESETS_PER_BANK + i;
            UpdateLeds(currentBank, currentPreset);
            SendMidiPc(currentPreset);
            
            buttons[i].actionPending = false;
            buttons[i].timeWhenPendingStarted = 0xffffffff;
        }
    }
}