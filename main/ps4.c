#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_system.h>
#include "btstack_config.h"
#include "btstack.h"
#include "stack/bt_types.h"
#include "osi/allocator.h"
#include "stack/gap_api.h"
#include "ps4.h"
#include "main.h"

static const uint8_t hid_cmd_payload_ps4_enable[] = { 0x43, 0x02 };


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

static void ps4_gap_send_hid(hid_cmd_t *hid_cmd, uint16_t channel, uint8_t len)
{
    uint8_t result;
    BT_HDR  *p_buf;

    p_buf = (BT_HDR *)osi_malloc(BT_DEFAULT_BUFFER_SIZE);

    if(!p_buf)
    {
        ESP_LOGE(PS4_TAG, "[%s] allocating buffer for sending the command failed", __func__);
    }

    p_buf->len = len + ( sizeof(*hid_cmd) - sizeof(hid_cmd->data) );
    p_buf->offset = L2CAP_MIN_OFFSET;

    memcpy ((uint8_t *)(p_buf + 1) + p_buf->offset, (uint8_t*)hid_cmd, p_buf->len);

    //result = GAP_ConnBTWrite(gap_handle_hidc, p_buf);
    result = l2cap_send(channel,(uint8_t*)p_buf,PS4_HID_BUFFER_SIZE);

    if (result == BT_PASS) 
    {
        ESP_LOGI(PS4_TAG, "[%s] sending command: success\n", __func__);
        //printf("[%s] sending command: success", __func__);
    }
    else {
        ESP_LOGE(PS4_TAG, "[%s] sending command: failed\n", __func__);
        //printf("[%s] sending command: success", __func__);
    }
}


static void ps4_command(ps4_cmd_t cmd, uint16_t channel)
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

    ps4_gap_send_hid(&hid_cmd, channel, len);
}

void ps4_set_led(uint8_t r, uint8_t g, uint8_t b, uint16_t channel)
{
    ps4_cmd_t cmd = { 0 };

    cmd.r = r;
    cmd.g = g;
    cmd.b = b;

    int ret = request_send_interrupt();
    int poll_status = 0;

    if (ret)
    {
        while(poll_status == 0)
        {
            poll_status = poll_send_interrupt();
            if (poll_status)
            {
                ps4_command(cmd, channel);
                reset_send_interrupt();
                printf("----PS4 set LED sent\n");
                break;
            }
        }
    }

   // ps4_command(cmd, channel);
}

void ps4_enable(uint16_t channel)
{
    uint16_t len = sizeof(hid_cmd_payload_ps4_enable);
    hid_cmd_t hid_cmd;

    hid_cmd.code = hid_cmd_code_set_report | hid_cmd_code_type_feature;
    hid_cmd.identifier = hid_cmd_identifier_ps4_enable;

    memcpy( hid_cmd.data, hid_cmd_payload_ps4_enable, len);

    //ps4_gap_send_hid(&hid_cmd, channel, len);

    int ret = request_send_interrupt();
   // int poll_status = 0;

    printf("RET ---- %d\n", ret);

    // if (ret)
    // {
    //     while(poll_status == 0)
    //     {
    //         poll_status = poll_send_interrupt();
    //         printf("poll statud %d",poll_status);
    //         if (poll_status)
    //         {
    //             ps4_gap_send_hid(&hid_cmd, channel, len);
    //             reset_send_interrupt();
    //             printf("----PS4 enable sent\n");
    //             break;
    //         }
    //     }
    // }

    //ps4_set_led(32, 32, 64, channel);
}
