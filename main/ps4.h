#ifndef PS4_H
#define PS4_H

#define PS4_REPORT_BUFFER_SIZE 77
#define PS4_HID_BUFFER_SIZE    50

#define PS4_TAG "PS4_SPP"

// typedef struct {
//     uint16_t          event;
//     uint16_t          len;
//     uint16_t          offset;
//     uint16_t          layer_specific;
//     uint8_t           data[];
// } BT_HDR;

// Output
enum hid_cmd_code {
    hid_cmd_code_set_report   = 0x50,
    hid_cmd_code_type_output  = 0x02,
    hid_cmd_code_type_feature = 0x03
};

enum hid_cmd_identifier {
    hid_cmd_identifier_ps4_enable  = 0xf4,
    hid_cmd_identifier_ps4_control = 0x11
};

typedef struct {
  uint8_t code;
  uint8_t identifier;
  uint8_t data[PS4_REPORT_BUFFER_SIZE];

} hid_cmd_t;

enum ps4_control_packet_index {
    ps4_control_packet_index_small_rumble = 5,
    ps4_control_packet_index_large_rumble = 6,

    ps4_control_packet_index_red = 7,
    ps4_control_packet_index_green = 8,
    ps4_control_packet_index_blue = 9,

    ps4_control_packet_index_flash_on_time = 10,
    ps4_control_packet_index_flash_off_time = 11
};

// Analog sticks
typedef struct {
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
} ps4_analog_stick_t;

typedef struct {
    uint8_t l2;
    uint8_t r2;
} ps4_analog_button_t;

typedef struct {
    ps4_analog_stick_t stick;
    ps4_analog_button_t button;
} ps4_analog_t;


// Buttons
typedef struct {
    uint8_t options  : 1;
	uint8_t l3       : 1;
	uint8_t r3       : 1;
    uint8_t share    : 1;

    uint8_t up       : 1;
    uint8_t right    : 1;
    uint8_t down     : 1;
    uint8_t left     : 1;

    uint8_t upright  : 1;
    uint8_t upleft   : 1;
    uint8_t downright: 1;
    uint8_t downleft : 1;

    uint8_t l2       : 1;
    uint8_t r2       : 1;
    uint8_t l1       : 1;
    uint8_t r1       : 1;

    uint8_t triangle : 1;
    uint8_t circle   : 1;
    uint8_t cross    : 1;
    uint8_t square   : 1;

    uint8_t ps       : 1;
    uint8_t touchpad : 1;
} ps4_button_t;


// Status flags
typedef struct {
    uint8_t battery;
    uint8_t charging : 1;
    uint8_t audio    : 1;
    uint8_t mic      : 1;
} ps4_status_t;


// Sensors
typedef struct {
    int16_t z;
} ps4_sensor_gyroscope_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} ps4_sensor_accelerometer_t;

typedef struct {
    ps4_sensor_accelerometer_t accelerometer;
    ps4_sensor_gyroscope_t gyroscope;
} ps4_sensor_t;

// Others
typedef struct {
    uint8_t smallRumble;
    uint8_t largeRumble;
    uint8_t r, g, b; // RGB
    uint8_t flashOn;
    uint8_t flashOff; // Time to flash bright/dark (255 = 2.5 seconds)
} ps4_cmd_t;

typedef struct {
    ps4_button_t button_down;
    ps4_button_t button_up;
    ps4_analog_t analog_move;
} ps4_event_t;

typedef struct {
    ps4_analog_t analog;
    ps4_button_t button;
    ps4_status_t status;
    ps4_sensor_t sensor;
} ps4_t;


void ps4_set_led(uint8_t r, uint8_t g, uint8_t b, uint16_t channel);
void ps4_enable(uint16_t channel);

#endif