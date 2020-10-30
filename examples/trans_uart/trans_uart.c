/*
 *
 *  GattLib - GATT Library example "trans_uart"  Transparent UART for RN4871
 *
 *  Copyright (C) 2016-2019  Olivier Martin <olivier@labapart.org>
 *  Copyright (C) 2020 Callender-Consulting <robin@callender-consulting.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

 /*
  *  This program works in conjunction with MicroChips RN4871 module, which
  *  operates in their "transparent uart" mode.  
  *  See MicroChip RN487x docs for details.
  */

#include <assert.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include "gattlib.h"

#define UART_CHARACTERISTIC_TX_UUID	"49535343-1e4d-4bd9-ba61-23c647249616"
#define UART_CHARACTERISTIC_RX_UUID	"49535343-8841-43f4-a8d4-ecbe34729bb3"
#define UART_CHARACTERISTIC_NX_UUID	"49535343-4c8a-39b3-2f49-511cff073b7e"

gatt_connection_t* m_connection;

static uint16_t tx_handle = 0;
static uint16_t rx_handle = 0;
static uint16_t nx_handle = 0;

static uuid_t uart_characteristic_tx_uuid;
static uuid_t uart_characteristic_rx_uuid;
static uuid_t uart_characteristic_nx_uuid;
static int tx_notify_started = 0;
static int nx_notify_started = 0;

GMainLoop* m_main_loop;
pthread_t tid;

void tx_notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) 
{
	int i;
	for(i = 0; i < data_length; i++) {
		if (data[i] == '\r') putchar('\n');
		printf("%c", data[i]);
	}
	fflush(stdout);
}

void nx_notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) 
{
	int i;
	for(i = 0; i < data_length; i++) {
		if (data[i] == '\r') putchar('\n');
		printf("%c", data[i]);
	}
	fflush(stdout);
}

static void usage(char *argv[])
{
	printf("%s <device_address>\n", argv[0]);
}

void int_handler(int dummy)
{
	if (tx_notify_started) {
		gattlib_notification_stop(m_connection, &uart_characteristic_tx_uuid);
		tx_notify_started = 0;
	}
	if (nx_notify_started) {
		gattlib_notification_stop(m_connection, &uart_characteristic_nx_uuid);
		nx_notify_started = 0;
	}
	g_main_loop_quit(m_main_loop);
	gattlib_disconnect(m_connection);
	exit(0);
}

void * send_thread(void *arg)
{
	char input[256];
	char * input_ptr;
	int ret, total_length, length = 0;

	fprintf(stderr, "type something, then [enter] to send.\n");

	while (1) {
		fgets(input, sizeof(input), stdin);

		// TX can only receive 20 bytes at a time
		input_ptr = input;
		for (total_length = strlen(input) + 1; total_length > 0; total_length -= length) {
			length = MIN(total_length, 20);
			ret = gattlib_write_without_response_char_by_handle(m_connection, tx_handle, input_ptr, length);
			if (ret) {
				fprintf(stderr, "Fail to send data to TX characteristic.\n");
				return NULL;
			}
			input_ptr += length;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	int i;

	if (argc != 2) {
		usage(argv);
		return 1;
	}

	m_connection = gattlib_connect(NULL, argv[1],
				       GATTLIB_CONNECTION_OPTIONS_LEGACY_BDADDR_LE_RANDOM |
				       GATTLIB_CONNECTION_OPTIONS_LEGACY_BT_SEC_LOW);
	if (m_connection == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}

	// Convert characteristics to their respective UUIDs
	ret = gattlib_string_to_uuid(UART_CHARACTERISTIC_TX_UUID,
		                         strlen(UART_CHARACTERISTIC_TX_UUID) + 1,
		                         &uart_characteristic_tx_uuid);
	if (ret) {
		fprintf(stderr, "Fail to convert characteristic TX to UUID.\n");
		return 1;
	}
	ret = gattlib_string_to_uuid(UART_CHARACTERISTIC_RX_UUID,
		                         strlen(UART_CHARACTERISTIC_RX_UUID) + 1,
		                         &uart_characteristic_rx_uuid);
	if (ret) {
		fprintf(stderr, "Fail to convert characteristic RX to UUID.\n");
		return 1;
	}
	ret = gattlib_string_to_uuid(UART_CHARACTERISTIC_NX_UUID, 
		                         strlen(UART_CHARACTERISTIC_NX_UUID) + 1,
		                         &uart_characteristic_nx_uuid);
	if (ret) {
		fprintf(stderr, "Fail to convert characteristic NX to UUID.\n");
		return 1;
	}

	// Look for handle for UART_CHARACTERISTIC_TX_UUID
	gattlib_characteristic_t* characteristics;
	int characteristic_count;
	ret = gattlib_discover_char(m_connection, &characteristics, &characteristic_count);
	if (ret) {
		fprintf(stderr, "Fail to discover characteristic.\n");
		return 1;
	}

	for (i = 0; i < characteristic_count; i++) {
		if (gattlib_uuid_cmp(&characteristics[i].uuid, &uart_characteristic_tx_uuid) == 0) {
			tx_handle = characteristics[i].value_handle;
		} else if (gattlib_uuid_cmp(&characteristics[i].uuid, &uart_characteristic_rx_uuid) == 0) {
			rx_handle = characteristics[i].value_handle;
		} else if (gattlib_uuid_cmp(&characteristics[i].uuid, &uart_characteristic_nx_uuid) == 0) {
			nx_handle = characteristics[i].value_handle;
		}
	}
	if (tx_handle == 0) {
		fprintf(stderr, "Fail to find TX characteristic.\n");
		return 1;
	} else if (rx_handle == 0) {
		fprintf(stderr, "Fail to find RX characteristic.\n");
		return 1;
	} else if (nx_handle == 0) {
		fprintf(stderr, "Fail to find NX characteristic.\n");
		return 1;
	}

	free(characteristics);

	// Register TX notification handler
	gattlib_register_notification(m_connection, tx_notification_cb, NULL);
	ret = gattlib_notification_start(m_connection, &uart_characteristic_tx_uuid);
	if (ret) {
		fprintf(stderr, "Fail to start TX notification.\n");
	} else {
		tx_notify_started = 1;
	}

	// Register NX notification handler
	gattlib_register_notification(m_connection, nx_notification_cb, NULL);
	ret = gattlib_notification_start(m_connection, &uart_characteristic_nx_uuid);
	if (ret) {
		fprintf(stderr, "Fail to start NX notification.\n");
	} else {
		nx_notify_started = 1;
	}

	// Register handler to catch Ctrl+C
	signal(SIGINT, int_handler);

	m_main_loop = g_main_loop_new(NULL, 0);

	pthread_create(&tid, NULL, send_thread, NULL);

	g_main_loop_run(m_main_loop);

	g_main_loop_unref(m_main_loop);

	return 0;
}
