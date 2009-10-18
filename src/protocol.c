/*
 * "protocol.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
 *
 * lulzNet is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your hs_option) any later version.
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

/* TODO: mutex to client connect/disconnect*/

#include <lulznet/lulznet.h>
#include <lulznet/types.h>

#include <lulznet/auth.h>
#include <lulznet/config.h>
#include <lulznet/log.h>
#include <lulznet/networking.h>
#include <lulznet/peer.h>
#include <lulznet/tap.h>
#include <lulznet/protocol.h>
#include <lulznet/xfunc.h>

/* Global var used to store packets send 
 * and recv during handshake
 * TODO: find right size 
 */
char packet[128];

void
send_banner (int fd)
{

  char banner[512];
  int len;

  sprintf (banner, "LulzNet. Version %s", VERSION);
  len = strlen (banner);
  write (fd, banner, len);
}

void
recv_banner (int fd)
{

  char banner[512];
  int len;

  len = read (fd, banner, 511);
  banner[len] = '\x00';
  info ("Recv Banner:\n%s", banner);

}

hs_opt_t *
server_handshake (SSL * ssl)
{
  hs_opt_t *hs_opt;

  hs_opt = (hs_opt_t *) xmalloc (sizeof (hs_opt_t));
  hs_opt->peer_username = (char *) xmalloc ((MAX_USERNAME_LEN + 1) * sizeof (char));

  hs_opt->user_ls = (user_ls_t *) xmalloc (sizeof (user_ls_t));
  hs_opt->net_ls = (net_ls_t *) xmalloc (sizeof (net_ls_t));

  /*
   * PROTOCOL!1!1ONE
   */

  /* Exchange peer username */
  if (!lulznet_server_user_exchange (ssl, hs_opt))
    return NULL;

  /* Recv hash and do authentication */
  if (!lulznet_server_auth (ssl, hs_opt))
    return NULL;

  debug2 ("Recving listening status");
  if (!xSSL_read (ssl, packet, sizeof (char), "listening status"))
    return NULL;

  /* Networks exchange */
  if (!lulznet_recv_network (ssl, hs_opt))
    return NULL;

  if (!lulznet_send_network (ssl, hs_opt))
    return NULL;

  /* User exchange */
  if (!lulznet_recv_userlist (ssl, hs_opt))
    return NULL;

  if (!lulznet_send_userlist (ssl))
    return NULL;

  return hs_opt;
}

hs_opt_t *
peer_handshake (SSL * ssl)
{

  hs_opt_t *hs_opt;

  hs_opt = (hs_opt_t *) xmalloc (sizeof (hs_opt_t));
  hs_opt->peer_username = (char *) xmalloc ((MAX_USERNAME_LEN + 1) * sizeof (char));

  hs_opt->user_ls = (user_ls_t *) xmalloc (sizeof (user_ls_t));
  hs_opt->net_ls = (net_ls_t *) xmalloc (sizeof (net_ls_t));

  /*
   * PROTOCOL!1!!ONE 
   */

  if (!lulznet_client_user_exchange (ssl, hs_opt))
    return NULL;

  if (!lulznet_client_auth (ssl))
    return NULL;

  /*
   * Hanshake
   */

  /* Peer tells remote peer if it's listening or not */
  /* we need to know this for routing */
  debug2 ("Sending listening status");
  if (opt.flags & LISTEN_MODE)
    packet[0] = 1;
  else
    packet[0] = 0;

  if (!xSSL_write (ssl, packet, sizeof (char), "listening status"))
    return NULL;

  /* Networks exchange */
  if (!lulznet_send_network (ssl, hs_opt))
    return NULL;

  if (!lulznet_recv_network (ssl, hs_opt))
    return NULL;

  /* User exchange */
  if (!lulznet_send_userlist (ssl))
    return NULL;

  if (!lulznet_recv_userlist (ssl, hs_opt))
    return NULL;

  return hs_opt;
}

int
lulznet_server_user_exchange (SSL * ssl, hs_opt_t * hs_opt)
{
  int rd_len;

  debug2 ("Recving username");
  if (!(rd_len = xSSL_read (ssl, hs_opt->peer_username, MAX_USERNAME_LEN, "username")))
    return FAIL;

  hs_opt->peer_username[rd_len] = '\x00';

  if (user_is_connected (hs_opt->peer_username) || (!strcmp (hs_opt->peer_username, opt.username)))
    {
      packet[0] = 0;
      xSSL_write (ssl, packet, 1, "user info");
      return FAIL;
    }

  packet[0] = 1;
  if (!xSSL_write (ssl, packet, 1, "user info"))
    return FAIL;

  /* And send its username */
  debug2 ("Sending username");
  if (!xSSL_write (ssl, opt.username, strlen (opt.username), "username"))
    return FAIL;

  return DONE;
}

int
lulznet_client_user_exchange (SSL * ssl, hs_opt_t * hs_opt)
{
  int rd_len;

  /* Peer send its username */
  debug2 ("Sending username");
  if (!xSSL_write (ssl, opt.username, strlen (opt.username), "username"))
    return FAIL;

  xSSL_read (ssl, packet, 1, "user info");
  if (packet[0] == 0)
    {
      error ("user is connected");
      return FAIL;
    }

  /* And recv remote peer username */
  debug2 ("Recving username");
  if (!(rd_len = xSSL_read (ssl, hs_opt->peer_username, (MAX_USERNAME_LEN), "username")))
    return FAIL;

  hs_opt->peer_username[rd_len] = '\x00';

  return DONE;
}

int
lulznet_server_auth (SSL * ssl, hs_opt_t * hs_opt)
{

  char hex_hash[16];
  char auth;
  /* Recv hash */
  debug2 ("Recving hash");
  if (!xSSL_read (ssl, hex_hash, 16, "hash"))
    return FAIL;

  /* Do authentication checking if hash match local credential file's hash */
  /* TODO: avoid cast */
  if (do_authentication (hs_opt->peer_username, (u_char *) hex_hash))
    {
      auth = AUTHENTICATION_SUCCESSFULL;
      debug2 ("Sending auth response (successfull)");
      if (!xSSL_write (ssl, &auth, sizeof (char), "auth response"))
	return FAIL;
    }
  else
    {
      auth = AUTHENTICATION_FAILED;
      debug2 ("Sending auth response (failed)");
      xSSL_write (ssl, &auth, sizeof (char), "auth response");
      return FAIL;
    }

  return DONE;
}

int
lulznet_client_auth (SSL * ssl)
{

  char *password;
  char *hex_hash;
  char auth;

  password = get_password ();

  hex_hash = (char *) calculate_md5 (password);

  /* Then send password's hash */
  debug2 ("Sending hash");
  if (!xSSL_write (ssl, hex_hash, 16, "hash"))
    {
      free (hex_hash);
      return FAIL;
    }

  free (hex_hash);

  /* And recv authentication response */
  debug2 ("Recving auth response");

  if (!xSSL_read (ssl, &auth, sizeof (char), "auth response"))
    return FAIL;

  debug2 ("Server response: %s (%x)", (auth ? "auth successfull" : "auth failed"), auth);

  if (auth == AUTHENTICATION_FAILED)
    {
      error ("Authentication failed");
      free (saved_password);
      saved_password = NULL;
      return FAIL;
    }
  return DONE;
}

int
lulznet_send_network (SSL * ssl, hs_opt_t * hs_opt)
{

  int i;
  net_ls_t *local_net_ls = get_user_allowed_networks (hs_opt->peer_username);

  debug2 ("Sending available network count");
  if (local_net_ls->count == 0)
    {
      debug2 ("Peer cannot access any networks");
      xSSL_write (ssl, &local_net_ls->count, sizeof (int), "network count");
      return FAIL;
    }

  if (!xSSL_write (ssl, &local_net_ls->count, sizeof (int), "network count"))
    return FAIL;

  /* TODO: add max remote peer capabilities */

  for (i = 0; i < local_net_ls->count; i++)
    {

      if (!xSSL_write (ssl, &local_net_ls->network[i], sizeof (int), "network list"))
	return FAIL;

      if (!xSSL_write (ssl, &local_net_ls->netmask[i], sizeof (int), "netmask list"))
	return FAIL;

    }

  return DONE;

}

int
lulznet_recv_network (SSL * ssl, hs_opt_t * hs_opt)
{
  int i;
  int rd_len;

  debug2 ("Recving available network count");
  if (!(rd_len = xSSL_read (ssl, &hs_opt->net_ls->count, sizeof (int), "network count")))
    return FAIL;

  if (hs_opt->net_ls->count == 0)
    {
      error ("No network available");
      return FAIL;
    }

  for (i = 0; i < hs_opt->net_ls->count && i < MAX_TAPS; i++)
    {

      if (!(rd_len = xSSL_read (ssl, &hs_opt->net_ls->network[i], sizeof (int), "network list")))
	return FAIL;

      if (!(rd_len = xSSL_read (ssl, &hs_opt->net_ls->netmask[i], sizeof (int), "netmask list")))
	return FAIL;
    }

  return DONE;
}

int
lulznet_send_userlist (SSL * ssl)
{
  int i;
  user_ls_t *user_ls;

  user_ls = get_userlist ();

  debug2 ("Sending peer count");
  if (!xSSL_write (ssl, &user_ls->count, sizeof (int), "peer count"))
    return FAIL;

  /* And send peers address */
  for (i = 0; i < user_ls->count; i++)
    {
      sprintf (packet, "%s", user_ls->user[i]);
      if (!xSSL_write (ssl, packet, strlen (packet), "user list"))
	return FAIL;

      if (!xSSL_write (ssl, &user_ls->address[i], sizeof (int), "address list"))
	return FAIL;
    }

  return DONE;
}

int
lulznet_recv_userlist (SSL * ssl, hs_opt_t * hs_opt)
{
  int i;
  int rd_len;

  if (!xSSL_read (ssl, &hs_opt->user_ls->count, sizeof (int), "peer count"))
    return FAIL;

  /* And recv peers info */
  for (i = 0; i < hs_opt->user_ls->count && i < MAX_PEERS; i++)
    {
      if (!(rd_len = xSSL_read (ssl, packet, MAX_USERNAME_LEN, "user list")))
	return FAIL;

      packet[rd_len] = '\x00';
      hs_opt->user_ls->user[i] = (char *) malloc ((rd_len + 1) * sizeof (char));
      strcpy (hs_opt->user_ls->user[i], packet);

      if (!(rd_len = xSSL_read (ssl, &hs_opt->user_ls->address[i], sizeof (int), "address list")))
	return FAIL;

    }
  return DONE;

}

user_ls_t *
get_userlist ()
{

  int i;
  peer_handler_t *peer;
  user_ls_t *user_ls;

  user_ls = (user_ls_t *) xmalloc (sizeof (user_ls_t));
  user_ls->count = 0;

  for (i = 0; i < peer_count; i++)
    {
      peer = peer_db + i;

      user_ls->user[user_ls->count] = peer->user;
      user_ls->address[user_ls->count] = peer->address;
      user_ls->count++;
    }

  return user_ls;
}
