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

#ifndef _WIFI_PARAMETERS_H
#define _WIFI_PARAMETERS_H

struct wifi_parameters {
	char gw_ip[4];
	char netmask[4];
	char start_ip[4];
	char end_ip[4];
	char ssid[22];
	char password[22];
};

#endif
