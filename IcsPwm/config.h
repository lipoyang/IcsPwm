// ICS-PWM Adapter configuration

// Serial port for debug
// (if no debug serial, please comment out.)
#define DEBUG_SERIAL    Serial

// Serial port for ICS bus
#define ICS_SERIAL      Serial1

// TX pin of ICS_SERIAL (for setting open drain)
#define PIN_ICS_TX      7

// Baud rate for ICS bus
#define ICS_BAUD        115200

// Number of Servos (1, 2, 4, 8)
#define SERVO_NUM   4

// Pin assignment (Servo PWM output)
#define PIN_SERVO_0     3
#define PIN_SERVO_1     5
#define PIN_SERVO_2     6
#define PIN_SERVO_3     9
#define PIN_SERVO_4     0 // not use
#define PIN_SERVO_5     0 // not use
#define PIN_SERVO_6     0 // not use
#define PIN_SERVO_7     0 // not use

