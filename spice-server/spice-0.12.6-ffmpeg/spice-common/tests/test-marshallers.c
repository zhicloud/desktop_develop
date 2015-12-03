#include <glib.h>
#include <string.h>
#include <marshaller.h>

#include <generated_test_marshallers.h>

static uint8_t expected_data[] = { 0x02, 0x00, 0x00, 0x00, /* data_size */
                                   0x08, 0x00, 0x00, 0x00, /* data offset */
                                   0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12, /* data */
                                   0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12, /* data */
};

int main(int argc, char **argv)
{
    SpiceMarshaller *marshaller;
    SpiceMsgMainShortDataSubMarshall *msg;
    size_t len;
    int free_res;
    uint8_t *data;

    msg = spice_malloc0(sizeof(SpiceMsgMainShortDataSubMarshall) + 2 * sizeof(uint64_t));
    msg->data_size = 2;
    msg->data[0] = 0x1234567890abcdef;
    msg->data[1] = 0x1234567890abcdef;

    marshaller = spice_marshaller_new();
    spice_marshall_msg_main_ShortDataSubMarshall(marshaller, msg);
    spice_marshaller_flush(marshaller);
    data = spice_marshaller_linearize(marshaller, 0, &len, &free_res);
    g_assert_cmpint(len, ==, G_N_ELEMENTS(expected_data));
    g_assert_true(memcmp(data, expected_data, len) == 0);
    if (free_res) {
        free(data);
    }
    spice_marshaller_destroy(marshaller);
    free(msg);

    return 0;
}
