// ICS-PWM Adapter

#include <stdint.h>
#include <stdbool.h>
#include <Servo.h>
#include <EEPROM.h>
#include "config.h"

// debug serial print
#ifdef DEBUG_SERIAL
#define DEBUG_PRINT(x)      DEBUG_SERIAL.print(x)
#define DEBUG_PRINTLN(x)    DEBUG_SERIAL.println(x)
#endif

// CMD(command)
#define CMD_POSITION    0x80    // POSITION command
#define CMD_ID          0xE0    // ID command

#define CMD_MASK        0xE0    // bit mask of command
#define ID_MASK         0x1F    // bit mask of ID
#define MSB_MASK        0x80    // bit mask of MSB

#if SERVO_NUM == 1
#define ID_GRP_MASK     0x1F
#elif SERVO_NUM == 2
#define ID_GRP_MASK     0x1E
#elif SERVO_NUM == 4
#define ID_GRP_MASK     0x1C
#else
#define ID_GRP_MASK     0x18
#endif

// SC(sub command) of ID command
#define SC_READ_ID      0x00    // read ID
#define SC_WRITE_ID     0x01    // write ID

// PWM output pins
static const int PIN_SERVOS[] = {
    PIN_SERVO_0, PIN_SERVO_1, PIN_SERVO_2, PIN_SERVO_3,
    PIN_SERVO_4, PIN_SERVO_5, PIN_SERVO_6, PIN_SERVO_7
};

// state of serial receiving
#define STATE_WAIT_COMMAND  0
#define STATE_WAIT_DATA     1
static int s_rxState = STATE_WAIT_COMMAND;

// ID of ICS
static uint8_t s_ID;

// servos
static Servo s_servos[SERVO_NUM];
// servo enable
static bool s_servoEnable[SERVO_NUM];

// received command and data
static uint8_t s_rxID;
static uint8_t s_rxCommand;
static uint8_t s_rxData[3];
static int s_rxDataSize;
static int s_rxDataCnt;

// setup
void setup()
{
    // serial port for debug
    DEBUG_SERIAL.begin(115200);
    // serial port for ICS bus
    ICS_SERIAL.begin(ICS_BAUD, SERIAL_8E1);
#if defined(__RL78__) || defined(__RX__)
    pinMode(PIN_ICS_TX, OUTPUT_OPENDRAIN);
#endif
    
    // read ID from EEPROM
    uint8_t eep_data[3];
    for(int i=0;i<3;i++){
        eep_data[i] = EEPROM.read(i);
    }
    if((eep_data[0] == 0xAA) && (eep_data[1] == 0x55)){
        s_ID = eep_data[2];
    }else{
        s_ID = 0x00;
        EEPROM.write(0, 0xAA);
        EEPROM.write(1, 0x55);
        EEPROM.write(2, s_ID);
    }

    // initialize servos
    for(int i=0;i<SERVO_NUM;i++){
        s_servoEnable[i] = false;
        pinMode(PIN_SERVOS[i], OUTPUT);
        digitalWrite(PIN_SERVOS[i], LOW); // TODO
        // s_servos[i].attach(PIN_SERVOS[i]);
        // s_servos[i].write(90);
    }
}

// set position
bool cmd_position()
{
    // sub ID (0 ... SERVO_NUM-1)
    uint8_t id_sub = s_rxID & (~ID_GRP_MASK);
    // position
    uint16_t pos = ((uint16_t)(s_rxData[0] & 0x7F) << 7) |
                    (uint16_t)(s_rxData[1] & 0x7F);
                    
    // turn on servo
    if( (!s_servoEnable[id_sub]) && (pos != 0)){
        s_servoEnable[id_sub] = true;
        s_servos[id_sub].attach(PIN_SERVOS[id_sub]);
    }
    // turn off servo
    if( s_servoEnable[id_sub] && (pos == 0)){
        s_servoEnable[id_sub] = false;
        s_servos[id_sub].detach();
        digitalWrite(PIN_SERVOS[id_sub], LOW); // TODO
    }
    
    // pos = 0 - 180 [deg]
    if(pos < 180){
        s_servos[id_sub].write(pos);
    }
    // pos = 500 - 2500 [usec]
    else if((500 <= pos) && (pos <= 2500)){
        s_servos[id_sub].writeMicroseconds(pos);
    }
    // pos = 7500 - 11500 [ICS compatible position]
    else if((7500 <= pos) && (pos <= 11500)){
        // 7500 - 11500 => 544 - 2400 [usec]
        int usec = map(pos, 7500, 11500, 544, 2400);
        s_servos[id_sub].writeMicroseconds(usec);
    }
    
    // response
    uint8_t response[3];
    response[0] = s_rxCommand & ~MSB_MASK;
    response[1] = s_rxData[0];
    response[2] = s_rxData[1];
    ICS_SERIAL.write(response, 3);
    
    return true;
}

// read / write ID
bool cmd_id()
{
    bool wait_response = false;
    
    // read
    if((s_rxData[0] == SC_READ_ID) &&
       (s_rxData[1] == SC_READ_ID) &&
       (s_rxData[2] == SC_READ_ID))
    {
        uint8_t response = CMD_ID | s_ID;
        ICS_SERIAL.write(response);
        wait_response = true;
    }
    // write
    else if((s_rxData[0] == SC_WRITE_ID) &&
            (s_rxData[1] == SC_WRITE_ID) &&
            (s_rxData[2] == SC_WRITE_ID))
    {
        ICS_SERIAL.write(s_rxCommand);
        s_ID = s_rxID & ID_GRP_MASK;
        EEPROM.write(2, s_ID);
        wait_response = true;
    }
    
    // wait for response
    if(wait_response){
        static const uint32_t RES_TIMEOUT = 5000; // [usec]
        uint32_t startTime = micros();
        while(ICS_SERIAL.available() <= 0){
            uint32_t now = micros();
            uint32_t elapsed = now - startTime;
            if(elapsed > RES_TIMEOUT) break;
        }
        // dummy read
        if(ICS_SERIAL.available() > 0){
            uint8_t c = ICS_SERIAL.read();
            (void)c;
        }
    }
}

// check received ICS command
bool checkReceivedCommand(uint8_t c)
{
    // command and ID
    s_rxCommand    = c & CMD_MASK;
    s_rxID         = c & ID_MASK;
    uint8_t id_grp = c & ID_GRP_MASK;
    
    // for me?
    if((s_rxCommand != CMD_ID) && (id_grp != s_ID)){
        return false;
    }
    
    bool ret = true;
    switch(s_rxCommand){
        // POSITION command
        case CMD_POSITION:
            s_rxDataSize = 2;
            s_rxDataCnt = 0;
            break;
        // ID command
        case CMD_ID:
            s_rxDataSize = 3;
            s_rxDataCnt = 0;
            break;
        // unsupported command
        default:
            ret = false;
            break;
    }
    return ret;
}

// check received ICS data
void checkReceivedData()
{
    switch(s_rxCommand){
        // POSITION command
        case CMD_POSITION:
            cmd_position();
            break;
        // ID command
        case CMD_ID:
            cmd_id();
            break;
    }
}

// main loop
void loop()
{
    // receiving data timeout
    static const uint32_t DATA_TIMEOUT = 5000; // [usec]
    static uint32_t startTime;
    if(s_rxState == STATE_WAIT_DATA){
        uint32_t now = micros();
        uint32_t elapsed = now - startTime;
        if(elapsed > DATA_TIMEOUT){
            s_rxState = STATE_WAIT_COMMAND;
        }
    }
    
    // receiving data
    while(ICS_SERIAL.available() > 0){
        uint8_t c = ICS_SERIAL.read();
        switch(s_rxState){
            // wait for ICS command
            case STATE_WAIT_COMMAND:
                if((c & MSB_MASK) != 0){
                    if(checkReceivedCommand(c)){
                        s_rxState = STATE_WAIT_DATA;
                        startTime = micros();
                    }
                }
                break;
            // wait for ICS data
            case STATE_WAIT_DATA:
                s_rxData[s_rxDataCnt] = c;
                s_rxDataCnt++;
                if(s_rxDataCnt >= s_rxDataSize){
                    checkReceivedData();
                    s_rxState = STATE_WAIT_COMMAND;
                }
                break;
            default:
                s_rxState = STATE_WAIT_COMMAND;
        }
    }
}

