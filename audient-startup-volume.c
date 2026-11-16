#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define AUDIENT_VID 0x2708
#define ID4_MKII_PID 0x0009

#define USB_REQUEST_TYPE_CLASS_INTERFACE_OUT 0x21
#define UAC2_CUR 0x01
#define UAC2_VOLUME_CONTROL 0x02
#define FEATURE_UNIT_ID 0x0A
#define DFU_INTERFACE_NUMBER 4

#define MAX_ATTEMPTS 30
#define RETRY_DELAY_SECONDS 1

static int set_channel_volume(libusb_device_handle *handle,
                              unsigned char channel,
                              int volume_db) {
    // UAC2 volume values are signed 16-bit, in 1/256 dB units.
    int16_t raw = (int16_t)(volume_db * 256);
    unsigned char payload[2] = {
        (unsigned char)(raw & 0xff),
        (unsigned char)(((uint16_t)raw >> 8) & 0xff)
    };

    uint16_t wValue = (uint16_t)((UAC2_VOLUME_CONTROL << 8) | channel);
    uint16_t wIndex = (uint16_t)((FEATURE_UNIT_ID << 8) | DFU_INTERFACE_NUMBER);

    return libusb_control_transfer(
        handle,
        USB_REQUEST_TYPE_CLASS_INTERFACE_OUT,
        UAC2_CUR,
        wValue,
        wIndex,
        payload,
        sizeof(payload),
        1500
    );
}

int main(int argc, char **argv) {
    long target_db = -20; // Safe default; override with one integer argument.

    if (argc > 1) {
        char *end = NULL;
        errno = 0;
        target_db = strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0') {
            fprintf(stderr, "Invalid volume '%s'. Use an integer from -127 to 0 dB.\n", argv[1]);
            return 2;
        }
    }

    if (target_db < -127 || target_db > 0) {
        fprintf(stderr, "Volume must be between -127 and 0 dB.\n");
        return 2;
    }

    libusb_context *context = NULL;
    int rc = libusb_init(&context);
    if (rc != LIBUSB_SUCCESS) {
        fprintf(stderr, "libusb_init failed: %s\n", libusb_error_name(rc));
        return 1;
    }

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
        libusb_device_handle *handle =
            libusb_open_device_with_vid_pid(context, AUDIENT_VID, ID4_MKII_PID);

        if (handle != NULL) {
            int ok = 1;
            // The iD4 MkII protocol applies the playback Feature Unit volume
            // to channels 0, 1 and 2 (master plus left/right).
            for (unsigned char channel = 0; channel <= 2; ++channel) {
                int transferred = set_channel_volume(handle, channel, (int)target_db);
                if (transferred != 2) {
                    fprintf(stderr,
                            "USB volume write failed on channel %u: %s (%d)\n",
                            channel,
                            transferred < 0 ? libusb_error_name(transferred) : "short transfer",
                            transferred);
                    ok = 0;
                    break;
                }
            }

            libusb_close(handle);
            libusb_exit(context);

            if (ok) {
                fprintf(stderr, "Set Audient iD4 MkII volume to %ld dB.\n", target_db);
                return 0;
            }
        } else {
            fprintf(stderr, "iD4 MkII not available yet (attempt %d/%d).\n",
                    attempt, MAX_ATTEMPTS);
        }

        if (attempt < MAX_ATTEMPTS) {
            sleep(RETRY_DELAY_SECONDS);
        }
    }

    libusb_exit(context);
    fprintf(stderr, "Could not set iD4 MkII volume. No successful USB write after %d attempts.\n",
            MAX_ATTEMPTS);
    return 1;
}
