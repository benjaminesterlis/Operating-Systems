#ifndef KCI_H
#define KCI_H

#include <linux/ioctl.h>

#define MAJOR_NUM 1234

/* Set the message of the device driver */

#define DEVICE_FILE_NAME "kci_dev"
#define DEVICE_R_NAME "kci_chardev"
#define IOCTL_CHIPER _IOW(MAJOR_NUM, 0, int)
#define IOCTL_SET_FD _IOW(MAJOR_NUM, 1, int)
#define IOCTL_SET_PID _IOW(MAJOR_NUM, 2, pid_t)
#define SUCCESS 0

#endif
