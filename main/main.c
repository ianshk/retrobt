
#define BTSTACK_FILE__ "main.c"

#include <inttypes.h>
#include <stdio.h>

#include "btstack_config.h"
#include "btstack.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "osi/allocator.h"
#include "ps4.h"


//#define MAX_ATTRIBUTE_VALUE_SIZE 300
#define MAX_ATTRIBUTE_VALUE_SIZE 375

// SDP
static uint8_t            hid_descriptor[MAX_ATTRIBUTE_VALUE_SIZE];
static uint16_t           hid_descriptor_len;

static uint16_t           hid_control_psm;
static uint16_t           hid_interrupt_psm;

static uint8_t            attribute_value[MAX_ATTRIBUTE_VALUE_SIZE];
static const unsigned int attribute_value_buffer_size = MAX_ATTRIBUTE_VALUE_SIZE;

// L2CAP
static uint16_t           l2cap_hid_control_cid;
static uint16_t           l2cap_hid_interrupt_cid;

// MBP 2016
static const char * remote_addr_string = "1C-66-6D-C8-3B-65"; //1C 66 6D C8 3B 65 

static bd_addr_t remote_addr;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t l2cap_control_packet_callback_registration;
static btstack_packet_callback_registration_t l2cap_interrupt_packet_callback_registration;

static const uint8_t hid_cmd_payload_ps4_enable[] = { 0x43, 0x02 };

typedef struct {
    uint16_t          event;
    uint16_t          len;
    uint16_t          offset;
    uint16_t          layer_specific;
    uint8_t           data[];
} BT_HDR;

static uint8_t sixaxis_init_state=0;

//#define BT_DEFAULT_BUFFER_SIZE          (4096 + 16)
//#define BT_DEFAULT_BUFFER_SIZE          79
//#define L2CAP_MIN_OFFSET    13     /* plus control(2), SDU length(2) */

// old 48
#define OUTPUT_REPORT_BUFFER_SIZE   77
#define HID_BUFFERSIZE              50

const unsigned char report_buffer[] = {
    0x80, 0x00, 0xff, 0x00, 0x00,
    0x00, 0x00, 0x20, 0x20, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00 
};

char lineBuffer[79];



static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

#define PS4_TAG "PS4_SPP"

void print_hid_cmd(hid_cmd_t *hid_cmd, uint8_t len)
{
//       uint8_t code;
//   uint8_t identifier;
//   uint8_t data[PS4_REPORT_BUFFER_SIZE];
    printf("len %d\n", len);
    printf("code: 0x%02x\n",hid_cmd->code);
    printf("identifier: 0x%02x\n", hid_cmd->identifier);
    

    for (int i = 0;i < PS4_REPORT_BUFFER_SIZE; i++)
    {
        printf("%02x ",hid_cmd->data[i]);
    }
    printf("\n");
}

void ps4_gap_send_hid( hid_cmd_t *hid_cmd, uint16_t channel, uint8_t len )
{
    print_hid_cmd(hid_cmd, len);
    memcpy(lineBuffer+2,report_buffer,OUTPUT_REPORT_BUFFER_SIZE);
    lineBuffer[0] = hid_cmd->code; //0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
    lineBuffer[1] = hid_cmd->identifier; //0x01;// Report ID
   //lineBuffer[11] |= (uint8_t)(((uint16_t)led & 0x0f) << 1);    

    l2cap_send(channel,(uint8_t*)lineBuffer,HID_BUFFERSIZE);
//     uint8_t result;
//     BT_HDR     *p_buf;

//     p_buf = (BT_HDR *)osi_malloc(BT_DEFAULT_BUFFER_SIZE);

//     if( !p_buf ){
//         ESP_LOGE(PS4_TAG, "[%s] allocating buffer for sending the command failed", __func__);
//     }

//     p_buf->len = len + ( sizeof(*hid_cmd) - sizeof(hid_cmd->data) );
//     p_buf->offset = L2CAP_MIN_OFFSET;

//     memcpy ((uint8_t *)(p_buf + 1) + p_buf->offset, (uint8_t*)hid_cmd, p_buf->len);

//    //  result = GAP_ConnBTWrite(gap_handle_hidc, p_buf);
//     l2cap_send(channel,p_buf->data,p_buf->len);

//     if (result == BT_PASS) {
//         ESP_LOGI(PS4_TAG, "[%s] sending command: success\n", __func__);
//         //printf("[%s] sending command: success", __func__);
//     }
//     else {
//         ESP_LOGE(PS4_TAG, "[%s] sending command: failed\n", __func__);
//         //printf("[%s] sending command: success", __func__);
//     }
}

void ps4Cmd( ps4_cmd_t cmd, uint16_t channel )
{
    hid_cmd_t hid_cmd = { .data = {0x80, 0x00, 0xFF} };
    uint16_t len = sizeof(hid_cmd.data);

    hid_cmd.code = hid_cmd_code_set_report | hid_cmd_code_type_output;
    hid_cmd.identifier = hid_cmd_identifier_ps4_control;

    hid_cmd.data[ps4_control_packet_index_small_rumble] = cmd.smallRumble; // Small Rumble
    hid_cmd.data[ps4_control_packet_index_large_rumble] = cmd.largeRumble; // Big rumble

    hid_cmd.data[ps4_control_packet_index_red] = cmd.r; // Red
    hid_cmd.data[ps4_control_packet_index_green] = cmd.g; // Green
    hid_cmd.data[ps4_control_packet_index_blue] = cmd.b; // Blue

    hid_cmd.data[ps4_control_packet_index_flash_on_time] = cmd.flashOn; // Time to flash bright (255 = 2.5 seconds)
    hid_cmd.data[ps4_control_packet_index_flash_off_time] = cmd.flashOff; // Time to flash dark (255 = 2.5 seconds)

    ps4_gap_send_hid( &hid_cmd, channel, len );
}

void ps4SetLed(uint8_t r, uint8_t g, uint8_t b, uint16_t channel)
{
    printf("ps4 set led\n");

    ps4_cmd_t cmd = { 0 };

    cmd.r = r;
    cmd.g = g;
    cmd.b = b;

    ps4Cmd(cmd, channel);
}

void ps4Enable(uint16_t channel)
{
    uint16_t len = sizeof(hid_cmd_payload_ps4_enable);
    hid_cmd_t hid_cmd;

    hid_cmd.code = hid_cmd_code_set_report | hid_cmd_code_type_feature;
    hid_cmd.identifier = hid_cmd_identifier_ps4_enable;

    memcpy( hid_cmd.data, hid_cmd_payload_ps4_enable, len);

    ps4_gap_send_hid( &hid_cmd, channel, len );
    ps4SetLed(32, 32, 64, channel);
}


static int set_sixaxis_led(uint16_t channel,int led)
{
   memcpy(lineBuffer+2,report_buffer,OUTPUT_REPORT_BUFFER_SIZE);
    lineBuffer[0] = 0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
    lineBuffer[1] = 0x11;// Report ID
   //lineBuffer[11] |= (uint8_t)(((uint16_t)led & 0x0f) << 1);    

   return l2cap_send(channel,(uint8_t*)lineBuffer,HID_BUFFERSIZE);
}


static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    des_iterator_t attribute_list_it;
    des_iterator_t additional_des_it;
    des_iterator_t prot_it;
    uint8_t       *des_element;
    uint8_t       *element;
    uint32_t       uuid;
    uint8_t        status;

    switch (hci_event_packet_get_type(packet)){
        case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
            if (sdp_event_query_attribute_byte_get_attribute_length(packet) <= attribute_value_buffer_size) {
                attribute_value[sdp_event_query_attribute_byte_get_data_offset(packet)] = sdp_event_query_attribute_byte_get_data(packet);
                if ((uint16_t)(sdp_event_query_attribute_byte_get_data_offset(packet)+1) == sdp_event_query_attribute_byte_get_attribute_length(packet)) {
                    switch(sdp_event_query_attribute_byte_get_attribute_id(packet)) {
                        case BLUETOOTH_ATTRIBUTE_PROTOCOL_DESCRIPTOR_LIST:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {                                    
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                des_iterator_init(&prot_it, des_element);
                                element = des_iterator_get_element(&prot_it);
                                if (!element) continue;
                                if (de_get_element_type(element) != DE_UUID) continue;
                                uuid = de_get_uuid32(element);
                                des_iterator_next(&prot_it);
                                switch (uuid){
                                    case BLUETOOTH_PROTOCOL_L2CAP:
                                        if (!des_iterator_has_more(&prot_it)) continue;
                                        de_element_get_uint16(des_iterator_get_element(&prot_it), &hid_control_psm);
                                        printf("HID Control PSM: 0x%04x\n", (int) hid_control_psm);
                                        break;
                                    default:
                                        break;
                                }
                            }
                            break;
                        case BLUETOOTH_ATTRIBUTE_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {                                    
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                for (des_iterator_init(&additional_des_it, des_element); des_iterator_has_more(&additional_des_it); des_iterator_next(&additional_des_it)) {                                    
                                    if (des_iterator_get_type(&additional_des_it) != DE_DES) continue;
                                    des_element = des_iterator_get_element(&additional_des_it);
                                    des_iterator_init(&prot_it, des_element);
                                    element = des_iterator_get_element(&prot_it);
                                    if (!element) continue;
                                    if (de_get_element_type(element) != DE_UUID) continue;
                                    uuid = de_get_uuid32(element);
                                    des_iterator_next(&prot_it);
                                    switch (uuid){
                                        case BLUETOOTH_PROTOCOL_L2CAP:
                                            if (!des_iterator_has_more(&prot_it)) continue;
                                            de_element_get_uint16(des_iterator_get_element(&prot_it), &hid_interrupt_psm);
                                            printf("HID Interrupt PSM: 0x%04x\n", (int) hid_interrupt_psm);
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                            break;
                        case BLUETOOTH_ATTRIBUTE_HID_DESCRIPTOR_LIST:
                            for (des_iterator_init(&attribute_list_it, attribute_value); des_iterator_has_more(&attribute_list_it); des_iterator_next(&attribute_list_it)) {
                                if (des_iterator_get_type(&attribute_list_it) != DE_DES) continue;
                                des_element = des_iterator_get_element(&attribute_list_it);
                                for (des_iterator_init(&additional_des_it, des_element); des_iterator_has_more(&additional_des_it); des_iterator_next(&additional_des_it)) {                                    
                                    if (des_iterator_get_type(&additional_des_it) != DE_STRING) continue;
                                    element = des_iterator_get_element(&additional_des_it);
                                    const uint8_t * descriptor = de_get_string(element);
                                    hid_descriptor_len = de_get_data_size(element);
                                    memcpy(hid_descriptor, descriptor, hid_descriptor_len);
                                    printf("HID Descriptor:\n");
                                    printf_hexdump(hid_descriptor, hid_descriptor_len);
                                }
                            }                        
                            break;
                        default:
                            break;
                    }
                }
            } else {
                fprintf(stderr, "SDP attribute value buffer size exceeded: available %d, required %d\n", attribute_value_buffer_size, sdp_event_query_attribute_byte_get_attribute_length(packet));
            }
            break;
            
        case SDP_EVENT_QUERY_COMPLETE:
            if (!hid_control_psm) {
                printf("HID Control PSM missing\n");
                break;
            }
            if (!hid_interrupt_psm) {
                printf("HID Interrupt PSM missing\n");
                break;
            }
            printf("Setup HID\n");
           // status = l2cap_create_channel(packet_handler, remote_addr, hid_control_psm, 48, &l2cap_hid_control_cid);
            status = l2cap_create_channel(packet_handler, remote_addr, hid_control_psm, 48, &l2cap_hid_control_cid);
            if (status){
                printf("Connecting to HID Control failed: 0x%02x\n", status);
            }
            break;
    }
}

static void l2cap_control_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   uint8_t   status;
   uint16_t  l2cap_cid;
   if (packet_type == HCI_EVENT_PACKET){
      switch(packet[0]){
      case L2CAP_EVENT_INCOMING_CONNECTION:
         printf("L2CAP_EVENT_INCOMING_CONNECTION\n");
         l2cap_accept_connection(channel);
         break;
      case L2CAP_EVENT_CHANNEL_OPENED:
        status = packet[2];
        if (status){
            printf("L2CAP Connection failed: 0x%02x\n", status);
            break;
        }
        status = l2cap_create_channel(packet_handler, remote_addr, hid_interrupt_psm, 48, &l2cap_hid_interrupt_cid);
            if (status){
                printf("Connecting to HID Control failed: 0x%02x\n", status);
                break;
            }
            printf("control - created interrupt channel\n");
                                
        
       //  ps4Enable(channel);
       //  sixaxis_interrupt_channel_id=channel;
         break;
      case L2CAP_EVENT_CHANNEL_CLOSED:
         printf("L2CAP_CHANNEL_CLOSED\n");
         //sixaxis_control_channel_id=0;
         sixaxis_init_state=0;
         //init_button_state();
         break;
      case DAEMON_EVENT_L2CAP_CREDITS:
      printf("-----daemon state\n");
         switch(sixaxis_init_state){
         case  1:
         printf("-----daemon state 1 control\n");
            sixaxis_init_state++;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
           // set_sixaxis_led(channel,1);
            break;
         default:
            break;
         }
         break;
        case L2CAP_EVENT_CAN_SEND_NOW:
            printf("can send now - control\n");
            break;
      default:
       //  printf("control l2cap:unknown(%02x)\n",packet[0]);
         break;
      }  
   }
      if (packet_type == L2CAP_DATA_PACKET){
                      printf("HID Control: ");
                printf_hexdump(packet, size);
   }
}

static void l2cap_interrupt_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   uint8_t   status;
   uint16_t  l2cap_cid;
   if (packet_type == HCI_EVENT_PACKET){
      switch(packet[0]){
      case L2CAP_EVENT_INCOMING_CONNECTION:
         printf("L2CAP_EVENT_INCOMING_CONNECTION\n");
         l2cap_accept_connection(channel);
         break;
      case L2CAP_EVENT_CHANNEL_OPENED:
        status = packet[2];
        if (status){
            printf("L2CAP Connection failed: 0x%02x\n", status);
            break;
        }

            printf("interrupt - HID Connection established\n");
        
         l2cap_request_can_send_now_event(channel);
       //  sixaxis_interrupt_channel_id=channel;
         break;
      case L2CAP_EVENT_CHANNEL_CLOSED:
         printf("L2CAP_CHANNEL_CLOSED\n");
         //sixaxis_interrupt_channel_id=0;
         sixaxis_init_state=0;
         //init_button_state();
         break;
      case DAEMON_EVENT_L2CAP_CREDITS:
        printf("daemon interrupt\n");
         switch(sixaxis_init_state){
         case  0:
         printf("-----daemon state 0 interrupt\n");
            sixaxis_init_state++;
           // enable_sixaxis(sixaxis_control_channel_id);
            break;
         default:
            break;
         }        break;
        case L2CAP_EVENT_CAN_SEND_NOW:
           // printf("can send now - interrutp\n");
            ps4SetLed(32, 32, 64, channel);
            break;
      default:
       //  printf("control l2cap:unknown(%02x)\n",packet[0]);
         break;
      }  
   }
   
   if (packet_type == L2CAP_DATA_PACKET){
                //       printf("HID Interrupt: ");
                // printf_hexdump(packet, size);
   }
}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    uint8_t   event;
    bd_addr_t event_addr;
    uint8_t   status;
    uint16_t  l2cap_cid;

    switch (packet_type) {
		case HCI_EVENT_PACKET:
            event = hci_event_packet_get_type(packet);
            switch (event) {            
                /* @text When BTSTACK_EVENT_STATE with state HCI_STATE_WORKING
                 * is received and the example is started in client mode, the remote SDP HID query is started.
                 */
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING){
                        printf("Start SDP HID query for remote HID Device.\n");
                        sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr, BLUETOOTH_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE_SERVICE);
                    }
                    break;

                /* LISTING_PAUSE */
                case HCI_EVENT_PIN_CODE_REQUEST:
					// inform about pin code request
                    printf("Pin code request - using '0000'\n");
                    hci_event_pin_code_request_get_bd_addr(packet, event_addr);
                    gap_pin_code_response(event_addr, "0000");
					break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // inform about user confirmation request
                    printf("SSP User Confirmation Request with numeric value '%"PRIu32"'\n", little_endian_read_32(packet, 8));
                    printf("SSP User Confirmation Auto accept\n");
                    break;

                /* LISTING_RESUME */

                // case L2CAP_EVENT_CHANNEL_OPENED: 
                //     status = packet[2];
                //     if (status){
                //         printf("L2CAP Connection failed: 0x%02x\n", status);
                //         break;
                //     }
                //     l2cap_cid  = little_endian_read_16(packet, 13);
                //     if (!l2cap_cid) break;
                //     if (l2cap_cid == l2cap_hid_control_cid){
                //         status = l2cap_create_channel(packet_handler, remote_addr, hid_interrupt_psm, 48, &l2cap_hid_interrupt_cid);
                //         if (status){
                //             printf("Connecting to HID Control failed: 0x%02x\n", status);
                //             break;
                //         }
                //     }                        
                //     if (l2cap_cid == l2cap_hid_interrupt_cid){
                //         printf("HID Connection established\n");
                //     }
                //     break;
                default:
                    break;
            }
            break;
        // case L2CAP_DATA_PACKET:
        //     // for now, just dump incoming data
        //     if (channel == l2cap_hid_interrupt_cid){
        //       //  hid_host_handle_interrupt_report(packet,  size);
        //     } else if (channel == l2cap_hid_control_cid){
        //         printf("HID Control: ");
        //         printf_hexdump(packet, size);
        //     } else {
        //         break;
        //     }
        default:
            break;
    }
}

int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){

    (void)argc;
    (void)argv;

    // Initialize L2CAP 
    printf("--------- Starting -----------\n");
    l2cap_init();

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_control_packet_callback_registration.callback = &l2cap_control_packet_handler;
    hci_add_event_handler(&l2cap_control_packet_callback_registration);
    l2cap_interrupt_packet_callback_registration.callback = &l2cap_interrupt_packet_handler; ;
    hci_add_event_handler(&l2cap_interrupt_packet_callback_registration);

    
    l2cap_register_packet_handler(packet_handler);
    l2cap_register_service(l2cap_control_packet_handler, PSM_HID_CONTROL, 160, LEVEL_2);
     l2cap_register_service(l2cap_interrupt_packet_handler, PSM_HID_INTERRUPT, 160, LEVEL_2);

    // Disable stdout buffering
    setbuf(stdout, NULL);

    // parse human readable Bluetooth address
    sscanf_bd_addr(remote_addr_string, remote_addr);

    // Turn on the device 
    hci_power_control(HCI_POWER_ON);
    return 0;
}

/* EXAMPLE_END */
