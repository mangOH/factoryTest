#include "uart.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/io.h>
#include <stdio.h>

#include <termios.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/signal.h>

int serial_fd = 0;
struct sigaction saio;
struct termios oldtio, newtio;

static bool data_available = false;
void signal_handler_IO(int status)
{
    data_available = true;
}

int serial_open(const char *serial_bus)
{
    int fd;

    fd = open(serial_bus, O_RDWR | O_NOCTTY | O_NONBLOCK);

    /* install the signal handler before making the device asynchronous */
    saio.sa_handler = signal_handler_IO;
    // saio.sa_mask = 0;
    saio.sa_flags = 0;
    saio.sa_restorer = NULL;
    sigaction(SIGIO, &saio, NULL);

    /* allow the process to receive SIGIO */
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, FASYNC);

    // fcntl (fd, F_SETFL, 0);

    // get the parameters
    tcgetattr(fd, &oldtio);
    tcgetattr(fd, &newtio);

    // Set the baud rates to 115200...
    cfsetispeed(&newtio, B115200);
    cfsetospeed(&newtio, B115200);
    // Enable the receiver and set local mode...
    newtio.c_cflag |= (CLOCAL | CREAD);
    // No parity (8N1):
    newtio.c_cflag &= ~PARENB;
    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;
    // enable hardware flow control (CNEW_RTCCTS)
    // newtio.c_cflag |= CRTSCTS;
    // if(hw_handshake)
    // Disable the hardware flow control for use with mangOH RED
    newtio.c_cflag &= ~CRTSCTS;
    // Disable software flow control
    newtio.c_iflag &= ~(IXON | IXOFF | IXANY);

    // set raw input
    newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    newtio.c_iflag &= ~(INLCR | ICRNL | IGNCR);

    // set raw output
    newtio.c_oflag &= ~OPOST;
    newtio.c_oflag &= ~OLCUC;
    newtio.c_oflag &= ~ONLRET;
    newtio.c_oflag &= ~ONOCR;
    newtio.c_oflag &= ~OCRNL;

    newtio.c_cc[VMIN] = 0;
    newtio.c_cc[VTIME] = 0;

    // Set the new newtio for the port...
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    return fd;
}
int serial_close(int serial_fd)
{
    // restore old serial configuration
    tcsetattr(serial_fd, TCSANOW, &oldtio);
    close(serial_fd);
    return 0;
}
int serial_read(int serial_fd, char *buffer, int buffer_size)
{
    int bytes_read = 0;
    bytes_read = read(serial_fd, buffer, buffer_size);
    return bytes_read;
}
int serial_write(int serial_fd, const char *data, int data_len)
{
    int wrote_bytes = 0;
    wrote_bytes = write(serial_fd, data, data_len);
    return wrote_bytes;
}
void serial_flush(int serial_fd)
{
    tcflush(serial_fd, TCIOFLUSH);
}

int serial_wait_for_data(int timeout)
{
    int timer = 0;
    while (data_available == false)
    {
        sleep(0.001);
        timer++;
        if (timer >= timeout)
            return -1;
    }
    LE_INFO("Spent %dms", timer);
    return 0;
}

