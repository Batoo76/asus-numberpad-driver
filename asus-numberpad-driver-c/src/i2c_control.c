#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <syslog.h>
#include "numberpad.h"

/* Send value to touchpad via I2C */
int i2c_send_value(int device_id, int device_addr, uint8_t value) {
    int fd;
    char path[32];
    uint8_t data[13] = {
        0x05, 0x00, 0x3d, 0x03, 0x06, 0x00, 0x07, 0x00,
        0x0d, 0x14, 0x03, value, 0xad
    };

    snprintf(path, sizeof(path), "/dev/i2c-%d", device_id);

    fd = open(path, O_RDWR);
    if (fd < 0) {
        syslog(LOG_DEBUG, "Failed to open I2C device %s, trying i2ctransfer fallback", path);
        goto fallback_i2ctransfer;
    }

    if (ioctl(fd, I2C_SLAVE, device_addr) < 0) {
        syslog(LOG_DEBUG, "Failed to set I2C slave address, trying i2ctransfer fallback");
        close(fd);
        goto fallback_i2ctransfer;
    }

    if (write(fd, data, sizeof(data)) != sizeof(data)) {
        syslog(LOG_DEBUG, "Failed to write I2C data, trying i2ctransfer fallback");
        close(fd);
        goto fallback_i2ctransfer;
    }

    close(fd);
    syslog(LOG_DEBUG, "I2C transfer successful via direct I2C API");
    return 0;

fallback_i2ctransfer:
    {
        char cmd[512];
        char hex_data[256] = "";
        int i;

        /* Build hex data string */
        for (i = 0; i < sizeof(data); i++) {
            char hex[8];
            snprintf(hex, sizeof(hex), "0x%02x ", data[i]);
            strcat(hex_data, hex);
        }

        snprintf(cmd, sizeof(cmd),
                 "i2ctransfer -f -y %d w%d@0x%02x %s",
                 device_id, (int)sizeof(data), device_addr, hex_data);

        syslog(LOG_DEBUG, "Trying I2C via i2ctransfer: %s", cmd);

        int ret = system(cmd);
        if (ret != 0) {
            syslog(LOG_ERR, "i2ctransfer failed with return code %d", ret);
            return -1;
        }

        syslog(LOG_DEBUG, "I2C transfer successful via i2ctransfer");
        return 0;
    }
}

