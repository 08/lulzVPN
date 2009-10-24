/*
   * "config.h" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
   *
   * lulzNet is free software; you can redistribute it and/or modify
   * it under the terms of the GNU General Public License as published by
   * the Free Software Foundation; either version 3 of the License, or
   * (at your option) any later version.
   *
   * lulzNet is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU General Public License
   * along with this program; if not, write to the Free Software
   * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   * MA 02110-1301, USA.
   */

#ifndef _LNET_CONFIG_H
#define _LNET_CONFIG_H

typedef struct
{

#define LISTEN_MODE 0x01
#define AUTH_SERVICE 0x02
#define INTERPEER_ACTIVE_MODE 0x04

  int flags;
  short connecting_port;
  short binding_port;
  char *connecting_address;
  char *binding_address;
  char *tap_address;
  char *tap_netmask;
  char *username;
} option_t;

#define CONFIG_FILE "/etc/lulznet/config"

/* Struct that holds options */
extern option_t opt;

/* parse console args */
void parse_args (int argc, char **argv);

/* parse config file */
void parse_config_file (char *filename);

/* initialize struct opt */
void set_default_options ();

/* check if something configuration is missing */
void check_empty_config_entry ();

#endif