#include "legato.h"
#include "interfaces.h"

/** 
 * Open Serial
 **/
int serial_open(const char *serial_bus);

/** 
 * Close Serial
 **/
int serial_close(int serial_fd);

/** 
 * Read Serial Data
 **/
int serial_read(int serial_fd, char *buffer, int buffer_size);

/** 
 * Read Serial Data
 **/
int serial_write(int serial_fd, const char *data, int data_len);

/** 
 * Read Serial Data
 **/
void serial_flush(int serial_fd);

/** 
 * Wait Serial Data
 **/
int serial_wait_for_data(int timeout);

/** 
 * Get Serial Signals
 * @fd: device name
 **/
le_result_t serial_signals_get(int fd);

/** 
 * Set Serial Signals
 * @fd: device name
 * @signal_name: Signal name
 * @value: Value of signal
 **/
le_result_t serial_signals_set(int fd, const char *signal_name, bool value);