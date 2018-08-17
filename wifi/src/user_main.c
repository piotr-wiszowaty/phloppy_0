/*
 * phloppy_0 - Commodore Amiga floppy drive emulator
 * Copyright (C) 2016-2018 Piotr Wiszowaty
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <osapi.h>
#include <user_interface.h>
#include <gpio.h>
#include <espconn.h>
#include "wifi_parameters.h"

#define MAX_CONNECTIONS	1

#define MAIN_PORT	4500
#define AUX_PORT	4501

#define MREQ	(1 << 4)
#define SREQ	(1 << 5)

struct espconn aux_conn;
esp_tcp aux_tcp;
struct espconn main_conn;
esp_tcp main_tcp;

char aux_buffer[80];
int aux_offset = 0;
char main_buffer[8192];
int main_rx_ri;
int main_rx_wi;
int main_tx_i;
char tx_blocked;

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set()
{
	switch (system_get_flash_size_map()) {
		case FLASH_SIZE_4M_MAP_256_256:
			return 128 - 5;
		case FLASH_SIZE_8M_MAP_512_512:
			return 256 - 5;
		case FLASH_SIZE_16M_MAP_512_512:
		case FLASH_SIZE_16M_MAP_1024_1024:
			return 512 - 5;
		case FLASH_SIZE_32M_MAP_512_512:
		case FLASH_SIZE_32M_MAP_1024_1024:
			return 1024 - 5;
		default:
			return 0;
	}
}

void
spi_txrx(char *buffer)
{
	gpio_output_set(MREQ, 0, 0, 0);
	while (!(GPIO_REG_READ(GPIO_IN_ADDRESS) & SREQ));
	spi_transaction(buffer, 64);
	gpio_output_set(0, MREQ, 0, 0);
	while (GPIO_REG_READ(GPIO_IN_ADDRESS) & SREQ);
}

void ICACHE_FLASH_ATTR
aux_recv_cb(void *arg, char *data, unsigned short length)
{
	struct espconn *conn = arg;

	if (data != NULL) {
		while (length--) {
			aux_buffer[aux_offset++ % sizeof(aux_buffer)] = *data++;
		}
	}
}

void ICACHE_FLASH_ATTR
aux_disconnect_cb(void *arg)
{
	struct espconn *conn = arg;

	os_printf("AUX disconnect %d.%d.%d.%d:%d\r\n", conn->proto.tcp->remote_ip[0],
			conn->proto.tcp->remote_ip[1], conn->proto.tcp->remote_ip[2],
			conn->proto.tcp->remote_ip[3], conn->proto.tcp->remote_port);
}
 
void ICACHE_FLASH_ATTR
aux_connect_cb(void *arg)
{
	struct espconn *conn = arg;

	espconn_send(conn, aux_buffer, sizeof(aux_buffer));
	aux_offset = 0;

	espconn_regist_recvcb(conn, aux_recv_cb);
	espconn_regist_disconcb(conn, aux_disconnect_cb);

	os_printf("AUX connect %d.%d.%d.%d:%d\r\n", conn->proto.tcp->remote_ip[0],
			conn->proto.tcp->remote_ip[1], conn->proto.tcp->remote_ip[2],
			conn->proto.tcp->remote_ip[3], conn->proto.tcp->remote_port);
}

sint8
comm_main(struct espconn *conn)
{
	sint8 result = 0;
	int i;

	while (main_tx_i != main_rx_ri) {
		if (main_buffer[main_tx_i]) {
			//os_printf("tx:%d\r\n", main_tx_i);
			result = espconn_send(conn, main_buffer + main_tx_i + 1, 63);
			if (result < 0) {
				break;
			} else {
				/*
				for (i = 0; i < 64; i++) {
					os_printf("%02x ", main_buffer[main_tx_i + i]);
				}
				os_printf("\r\n");
				*/
				main_tx_i = (main_tx_i + 64) & (sizeof(main_buffer) - 1);
			}
		} else {
			main_tx_i = (main_tx_i + 64) & (sizeof(main_buffer) - 1);
		}
	}
	return result;
}

int
abs(int x)
{
	return x < 0 ? -x : x;
}

void
main_recv_cb(void *arg, char *data, unsigned short length)
{
	struct espconn *conn = arg;
	sint8 result;

	if (data == NULL) {
		return;
	}

	while (length--) {
		main_buffer[main_rx_wi++] = *data++;
		main_rx_wi &= sizeof(main_buffer) - 1;
		if (abs(main_rx_wi - main_rx_ri) >= 64) {
			spi_txrx(main_buffer + main_rx_ri);
			main_rx_ri = (main_rx_ri + 64) & (sizeof(main_buffer) - 1);
			//os_printf("rx:%d\r\n", main_rx_ri);
			if (!tx_blocked) {
				if (comm_main(conn) < 0) {
					tx_blocked = 1;
					espconn_recv_hold(conn);
					os_printf("tx:block\r\n");
				}
			}
		}
	}
}

void ICACHE_FLASH_ATTR
main_sent_cb(void *arg)
{
	struct espconn *conn = arg;

	if (comm_main(conn) == 0 && main_rx_ri == main_tx_i && tx_blocked) {
		tx_blocked = 0;
		espconn_recv_unhold(conn);
		os_printf("tx:unblock\r\n");
	}
}

void ICACHE_FLASH_ATTR
main_disconnect_cb(void *arg)
{
	struct espconn *conn = arg;

	os_printf("MAIN disconnect %d.%d.%d.%d:%d\r\n", conn->proto.tcp->remote_ip[0],
			conn->proto.tcp->remote_ip[1], conn->proto.tcp->remote_ip[2],
			conn->proto.tcp->remote_ip[3], conn->proto.tcp->remote_port);
}
 
void ICACHE_FLASH_ATTR
main_connect_cb(void *arg)
{
	struct espconn *conn = arg;

	main_rx_ri = 0;
	main_rx_wi = 0;
	main_tx_i = 0;
	tx_blocked = 0;

	espconn_regist_recvcb(conn, main_recv_cb);
	espconn_regist_sentcb(conn, main_sent_cb);
	espconn_regist_disconcb(conn, main_disconnect_cb);

	os_printf("MAIN connect %d.%d.%d.%d:%d\r\n", conn->proto.tcp->remote_ip[0],
			conn->proto.tcp->remote_ip[1], conn->proto.tcp->remote_ip[2],
			conn->proto.tcp->remote_ip[3], conn->proto.tcp->remote_port);
}

void ICACHE_FLASH_ATTR
user_init()
{
	struct softap_config config;
	char buffer[64];
	struct wifi_parameters *wp = (struct wifi_parameters *) (buffer + 1);
	int i;

	os_printf("Phloppy_0\r\n");

	gpio_output_set(0, MREQ, MREQ, 0);

	spi_init();
	spi_tx_byte_order(0);
	spi_rx_byte_order(0);
	os_printf("SPI initialized\r\n");

	for (i = 0; i < sizeof(buffer); i++) {
		buffer[i] = 0;
	}
	buffer[0] = 0xc0;	// END
	buffer[1] = 0x80;	// SETUP_WIFI
	buffer[2] = 0xc0;	// END
	buffer[3] = 0x00;	// NOP
	spi_txrx(buffer);
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	spi_txrx(buffer);

	os_printf("SSID: %s, passwd: %s\r\n", wp->ssid, wp->password);

	wifi_set_opmode(SOFTAP_MODE);
	bzero(&config, sizeof(config));
	strcpy(config.ssid, wp->ssid);
	strcpy(config.password, wp->password);
	config.authmode = AUTH_WPA_WPA2_PSK;
	config.ssid_len = 0;
	config.max_connection = 2;
	wifi_softap_set_config(&config);

	os_printf("WiFi initialized\r\n");

	bzero(&main_conn, sizeof(struct espconn));
	bzero(&main_tcp, sizeof(esp_tcp));
	main_tcp.local_port = MAIN_PORT;
	main_conn.type = ESPCONN_TCP;
	main_conn.state = ESPCONN_NONE;
	main_conn.proto.tcp = &main_tcp;
	espconn_regist_connectcb(&main_conn, main_connect_cb);
	i = espconn_accept(&main_conn);
	os_printf("MAIN accept %d\r\n", i);

	bzero(aux_buffer, sizeof(aux_buffer));

	bzero(&aux_conn, sizeof(struct espconn));
	bzero(&aux_tcp, sizeof(esp_tcp));
	aux_tcp.local_port = AUX_PORT;
	aux_conn.type = ESPCONN_TCP;
	aux_conn.state = ESPCONN_NONE;
	aux_conn.proto.tcp = &aux_tcp;
	espconn_regist_connectcb(&aux_conn, aux_connect_cb);
	i = espconn_accept(&aux_conn);
	os_printf("AUX accept %d\r\n", i);
}
