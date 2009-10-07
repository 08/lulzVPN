/*
 * "shell.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
 *
 * lulzNet is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lulzNet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
*/

#include "headers/lulznet.h"

void
peer_preconnect (sh_cmd * cmd)
{
  int address;
  unsigned short port;

  if (cmd->argc < 1)
    shell_msg ("Usage: peer connect address [port] )");
  else if (cmd->argc == 1)
    {
      address = lookup_address (cmd->argv[0]);
      peer_connect (address, 7890);
    }
  else
    {
      address = xinet_pton (cmd->argv[0]);
      port = atoi (cmd->argv[1]);
      peer_connect (address, port);
    }

}

void
peer_list ()
{
  int i;
  int j;
  int max_fd;
  int tmp;
  char address[16];
  char network[16];
  char netmask[16];

  peer_handler_t *peer;

  max_fd = get_max_peer_fd ();

  for (i = 3; i <= max_fd; i++)
    {
      peer = get_fd_related_peer (i);
      if (peer != NULL)
	if (peer->flags & ACTIVE_PEER)
	  {
	    tmp = peer->address;
	    inet_ntop (AF_INET, &tmp, address, 16);

	    printf ("%s:\n\t[*] filedescriptor: %d\n\t[*] address: %s", peer->user, i, address);
	    printf ("\n\t[*] available networks: %d\n", 1);

	    for (j = 0; j < peer->nl->count; j++)
	      {

		inet_ntop (AF_INET, &peer->nl->network[j], network, 16);
		inet_ntop (AF_INET, &peer->nl->netmask[j], netmask, 16);

		printf ("\t\t[*] network:%s netmask:%s\n", network, netmask);
	      }
	    printf ("\n");
	  }
    }
}

void
peer_kill (sh_cmd * cmd)
{
  if (cmd->argc != 2)
    shell_msg ("Usage: peer kill fd");
  else if (!(is_active_peer_fd (atoi (cmd->argv[1]))))
    shell_msg ("Invalid fd specified");
  else
    peer_disconnect (atoi (cmd->argv[1]));
}

void
tap_list ()
{
  int i;
  int max_fd;
  int n_address;
  int n_netmask;
  char p_address[ADDRESS_LEN];
  char p_netmask[ADDRESS_LEN];

  tap_handler_t *tap;

  max_fd = get_max_fd ();

  for (i = 3; i <= max_fd; i++)
    {
      tap = get_fd_related_tap (i);
      if (tap != NULL)
	if (tap != NULL)
	  if (tap->flags & ACTIVE_TAP)
	    {
	      printf ("%s:\n\t[*] filedecriptor: %d\n", tap->device, tap->fd);
	      n_address = tap->address;
	      n_netmask = tap->netmask;
	      inet_ntop (AF_INET, &n_address, p_address, ADDRESS_LEN);
	      inet_ntop (AF_INET, &n_netmask, p_netmask, ADDRESS_LEN);
	      shell_msg ("\t[*] address: %s netmask: %s\n", p_address, p_netmask);
	    }
    }
}


sh_cmd *
preparse_command (char *line)
{

  sh_cmd *cmd;
  int line_len;
  int tmp_len;
  int parsed_bytes;

  cmd = xmalloc (sizeof (sh_cmd));
  line_len = strlen (line);
  parsed_bytes = 0;

  /* Read command */
  sscanf (line, "%32s", cmd->command);
  tmp_len = strlen (cmd->command) + 1;

  line += tmp_len;
  parsed_bytes += tmp_len;

  /* Read arguments */
  for (cmd->argc = 0; cmd->argc < 4 && parsed_bytes < line_len; cmd->argc++)
    {
      sscanf (line, "%128s", cmd->argv[cmd->argc]);
      tmp_len = strlen (cmd->argv[cmd->argc]) + 1;

      line += tmp_len;
      parsed_bytes += tmp_len;
    }

  return cmd;
}

void
parse_command (sh_cmd * cmd)
{

  /* peer command */
  if (!strcmp (cmd->command, "peer"))
    {
      if (!cmd->argc)
	shell_msg ("Usage: peer [ list | kill ]");
      else if (!strcmp (cmd->argv[0], "list"))
	peer_list ();
      else if (!strcmp (cmd->argv[0], "kill"))
	peer_kill (cmd);
      else
	error ("Unknow arg");
    }

  /* tap command */
  else if (!strcmp (cmd->command, "tap"))
    {
      if (!cmd->argc)
	shell_msg ("Usage: tap [ list | add | del ]");
      else if (!strcmp (cmd->argv[0], "list"))
	tap_list ();
      else if (!strcmp (cmd->argv[0], "add"))
	{
	  /* TODO: add all stuff */
	}
      else if (!strcmp (cmd->argv[0], "del"))
	{
	  /* TODO: add all stuff */
	}
      else
	error ("Unknow arg");
    }

  /* connect peer command */
  else if (!strcmp (cmd->command, "connect"))
    {
      if (!cmd->argc)
	shell_msg ("Usage: connect [address] [port]");
      else
	peer_preconnect (cmd);
    }

  /* useless command */
  else if (!strcmp (cmd->command, "whoami"))
    printf ("%s\n", opt.username);

  /* quit command */
  else if (!strcmp (cmd->command, "quit"))
    exit_lulznet ();

  /* invalid command */
  else
    shell_msg ("%s: command not found.", cmd->command);
}

void
start_shell ()
{
  char *line = (char *) NULL;
  sh_cmd *cmd;

  printf ("\n");
  sleep (1);
  while (TRUE)
    {
      line = readline ("[lulznet] ");
      if (line && *line)
	{
	  if ((cmd = preparse_command (line)))
	    {
	      parse_command (cmd);
	      add_history (line);
	    }

	  free (cmd);
	}

    }
}
