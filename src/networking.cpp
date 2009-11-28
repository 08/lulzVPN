/*
 * "networking.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

#include <lulznet/lulznet.h>

#include <lulznet/auth.h>
#include <lulznet/config.h>
#include <lulznet/log.h>
#include <lulznet/networking.h>
#include <lulznet/peer.h>
#include <lulznet/protocol.h>
#include <lulznet/packet.h>
#include <lulznet/tap.h>

SSL_CTX *Network::Client::ssl_ctx;
SSL_CTX *Network::Server::ssl_ctx;

fd_set Network::master;
pthread_t Network::Server::select_t;
int Network::free_fd_flag;

void Network::Server::SslInit ()
{
  Network::Server::ssl_ctx = SSL_CTX_new (SSLv23_server_method ());

  if (!Network::Server::ssl_ctx)
    Log::Fatal ("Failed to do SSL CTX new");

#ifdef DEBUG
  Log::Debug2 ("Loading SSL certificate");
#endif
  if (SSL_CTX_use_certificate_file
      (Network::Server::ssl_ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0)
    Log::Fatal ("Failed to load SSL certificate %s", CERT_FILE);

#ifdef DEBUG
  Log::Debug2 ("Loading SSL private key");
#endif
  if (SSL_CTX_use_PrivateKey_file
      (Network::Server::ssl_ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0)
    Log::Fatal ("Failed to load SSL private key %s", KEY_FILE);
}

void Network::Client::SslInit ()
{
  Network::Client::ssl_ctx = SSL_CTX_new (SSLv23_client_method ());
}

void *Network::Server::ServerLoop (void *arg __attribute__ ((unused)))
{

  int listen_sock;
  int peer_sock;
  int on = 1;
  SSL *peer_ssl;
  char peer_address[ADDRESS_LEN + 1];
  struct sockaddr_in server;
  struct sockaddr_in peer;
  socklen_t addr_size;
  pthread_t connect_queue_t;
  hs_opt_t hs_opt;
  Peers::Peer * new_peer;

  if ((listen_sock = socket (PF_INET, SOCK_STREAM, 0)) == -1)
    Log::Fatal ("cannot create socket");

#ifdef DEBUG
  Log::Debug2 ("listen_sock (fd %d) created", listen_sock);
#endif
  if (setsockopt (listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) ==
      -1)
    Log::Error ("setsockopt SO_REUSEADDR: %s", strerror (errno));

  server.sin_family = AF_INET;
  server.sin_port = htons (options.binding_port ());
  server.sin_addr.s_addr = INADDR_ANY;	/*(server_opt->binding_address); */
  memset (&(server.sin_zero), '\0', 8);

#ifdef DEBUG
  Log::Debug2 ("Binding port %d", PORT);
#endif
  if (bind
      (listen_sock, (struct sockaddr *) &server,
       sizeof (struct sockaddr)) == -1)
    Log::Fatal ("cannot binding to socket");

#ifdef DEBUG
  Log::Debug1 ("Listening");
#endif
  if (listen (listen_sock, MAX_ACCEPTED_PEERS_CONNECTIONS) == -1)
    Log::Fatal ("cannot listen");

  addr_size = sizeof (struct sockaddr_in);

  /* @TODO while(available_connection()) else sleep && goto! */
  while (1)
    {
      if ((peer_sock =
             accept (listen_sock, (struct sockaddr *) &peer, &addr_size)) == -1)
        Log::Fatal ("cannot accept");

      Protocol::SendBanner (peer_sock);

      if ((peer_ssl = SSL_new (Network::Server::ssl_ctx)) != NULL)
        {
          SSL_set_fd (peer_ssl, peer_sock);
#ifdef DEBUG
          Log::Debug2 ("SSL Handshake");
#endif
          if (SSL_accept (peer_ssl) > 0)
            {
              if (Protocol::server::Handshake (peer_ssl, &hs_opt))
                {
                  pthread_mutex_lock (&Peers::db_mutex);

                  new_peer = new Peers::Peer (peer_sock, peer_ssl, hs_opt.peer_username,
                                              peer.sin_addr.s_addr, hs_opt.net_ls,
                                              INCOMING_CONNECTION);
                  inet_ntop (AF_INET, &peer.sin_addr.s_addr, peer_address,
                             ADDRESS_LEN);
                  Log::Info ("Connection accepted from %s (fd %d)", peer_address,
                             peer_sock);

                  /* Set routing */
                  Taps::set_system_routing (new_peer, ADD_ROUTING);

                  pthread_mutex_unlock (&Peers::db_mutex);

                  pthread_create (&connect_queue_t, NULL, check_connections_queue,
                                  &hs_opt.user_ls);
                  pthread_join (connect_queue_t, NULL);
                }
              else
                {
                  Log::Error("Cannot complete handshake");
                  SSL_free (peer_ssl);
                  close (peer_sock);
                }
            }
          else
            {
              Log::Error ("Cannot complete SSL handshake");
              close (peer_sock);
            }
        }
      else
        {

          Log::Error ("Cannot create new SSL");
          close (peer_sock);
        }
    }
  return NULL;
}

int Network::LookupAddress (std::string address)
{

  struct hostent *host_info;

#ifdef DEBUG
  Log::Debug1 ("Looking up client %s", address.c_str ());
#endif
  host_info = gethostbyname (address.c_str ());

  if (host_info == NULL)
    {
      Log::Error ("Cannot lookup hostname", 1);
      return 0;
    }

  return *((int *) host_info->h_addr);

}

void Network::Client::PeerConnect (int address, short port)
{

  struct sockaddr_in peer;
  int peer_sock;
  SSL *peer_ssl;
  hs_opt_t hs_opt;

  pthread_t connect_queue_t;
  Peers::Peer * new_peer;

  /* check if is there any free Peer */
  if (Peers::conections_to_peer == MAX_CONNECTIONS_TO_PEER)
    {
      Log::Error ("Exceded max connections to peer");
      return;
    }

  if ((peer_sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      Log::Error ("cannot create socket", 1);
      return;
    }

#ifdef DEBUG
  Log::Debug2 ("peer_sock (fd %d) created", peer_sock);
#endif

  peer.sin_family = AF_INET;
  peer.sin_port = htons (port ? port : PORT);
  peer.sin_addr.s_addr = address;
  memset (&(peer.sin_zero), '\0', 8);

  if (connect (peer_sock, (struct sockaddr *) &peer, sizeof (peer)) == -1)
    {
      Log::Error ("Cannot connect", 1);
      return;
    }

  Protocol::RecvBanner (peer_sock);

  if ((peer_ssl = SSL_new (Network::Client::ssl_ctx)) != NULL)
    {
      SSL_set_fd (peer_ssl, peer_sock);
#ifdef DEBUG
      Log::Debug2 ("SSL Handshake");
#endif
      if (SSL_connect (peer_ssl) > 0)
        {
          if (Network::VerifySslCert (peer_ssl))
            {
              if (Protocol::client::Handshake (peer_ssl, &hs_opt))
                {
                  pthread_mutex_lock (&Peers::db_mutex);

                  new_peer =
                    new Peers::Peer (peer_sock, peer_ssl, hs_opt.peer_username, address,
                                     hs_opt.net_ls, OUTGOING_CONNECTION);
                  Log::Info ("Connected");

                  Taps::set_system_routing (new_peer, ADD_ROUTING);

                  pthread_mutex_unlock (&Peers::db_mutex);

                  pthread_create (&connect_queue_t, NULL,
                                  Network::check_connections_queue, &hs_opt.user_ls);
                  pthread_join (connect_queue_t, NULL);
                  /* XXX: check
                  delete hs_opt.user_ls.user;
                  delete hs_opt.user_ls.address;
                  delete hs_opt.net_ls.address;
                  delete hs_opt.net_ls.network;
                  delete hs_opt.net_ls.netmask; */
                }
              else
                {
                  Log::Error ("Cannot complete lulznet handshake");
                  SSL_free (peer_ssl);
                  close (peer_sock);
                }

            }
          else
            {
              Log::Error ("Cannot verify host identity");
              SSL_free (peer_ssl);
              close (peer_sock);
            }
        }
      else
        {
          Log::Error ("Cannot complete SSL handshake");
          SSL_free (peer_ssl);
          close (peer_sock);
        }
    }
  else
    {
      Log::Error ("Cannot creane new SSL");
      close (peer_sock);
    }
}

void *Network::Server::SelectLoop (void __attribute__ ((unused)) * arg)
{
  Packet packet;
  int ret;
  fd_set read_select;
  int max_fd;
  int i;
  Peers::Peer * peer;
  Taps::Tap * tap;

  int dont_close_flag = 1;

  while (dont_close_flag)
    {
      pthread_mutex_lock (&Peers::db_mutex);
      read_select = Network::master;
      free_fd_flag = 0;
      max_fd = (Peers::max_fd > Taps::max_fd ? Peers::max_fd : Taps::max_fd);
      pthread_mutex_unlock (&Peers::db_mutex);

      ret = select (max_fd + 1, &read_select, NULL, NULL, NULL);

      pthread_mutex_lock (&Peers::db_mutex);
      if (ret == -1)
        Log::Fatal ("Select Log::Error");
      else
        {
          /* 0,1 and 2 are stdin-out-err and we don't care about them */
          for (i = 0; i < Peers::count; i++)
            {
              peer = Peers::db[i];
              if (peer->isActive () && peer->isReadyToRead(&read_select))
                {
                  /* Read from it */
                  if ( *peer >> &packet )
                    {
                      switch (packet.buffer[0])
                        {
                        case DATA_PACKET:
                          Network::Server::ForwardToTap (&packet);
                          break;
                        case CONTROL_PACKET:
                          if (packet.buffer[1] == CLOSE_CONNECTION)
                            {
#ifdef DEBUG
                              Log::Debug3 ("control_packet: closing connection");
#endif
                              free_fd_flag = 1;
                              peer->setClosing ();
                            }
                          else
                            Log::Error ("Unknow control flag");
                          break;
                        }
                    }
                  else
                    free_fd_flag = TRUE;
                }
            }
          if (free_fd_flag)
            Peers::FreeNonActive ();

          for (i = 0; i < Taps::count; i++)
            {
              tap = Taps::db[i];
              if (tap->isActive() && tap->isReadyToRead(&read_select))
                {
                  if (*tap >> &packet)
                    Network::Server::ForwardToPeer (&packet);
                }
            }
        }

      /* When the cycle is end functions can modify the fd_db structure */
      pthread_mutex_unlock (&Peers::db_mutex);
    }
  return NULL;
}

void Network::Server::RestartSelectLoop ()
{
#ifdef DEBUG
  Log::Debug2 ("Restarting select()");
#endif
  if (Network::Server::select_t != (pthread_t) NULL)
    {
      if (pthread_cancel (Network::Server::select_t))
        Log::Fatal ("Cannot cancel select thread");
      else
        pthread_create (&Network::Server::select_t, NULL,
                        Network::Server::SelectLoop, NULL);
    }
}

inline void Network::Server::ForwardToTap (Network::Packet * packet)
{

  int i;
  int n_addr;

  n_addr = PacketInspection::get_destination_ip(packet);

  for (i = 0; i < Taps::count; i++)
    if (Taps::db[i]->isActive())
      *Taps::db[i] << packet;

#ifdef DEBUG
  Log::Dump (packet->buffer, packet->length);
#endif
}

inline void Network::Server::ForwardToPeer (Network::Packet * packet)
{

  int i;
  int n_addr;

  n_addr = PacketInspection::get_destination_ip(packet);
  packet->buffer[0] = DATA_PACKET;

  for (i = 0; i < Peers::count; i++)
    if (Peers::db[i]->isActive())
      if (Peers::db[i]->isRoutableAddress(n_addr))
        *Peers::db[i] << packet;

#ifdef DEBUG
  Log::Dump (packet->buffer, packet->length);
#endif
}

int Network::VerifySslCert (SSL * ssl)
{
  char *fingerprint;
  char answer;

  if (SSL_get_verify_result (ssl) != X509_V_OK)
    {
      fingerprint = Auth::Crypt::GetFingerprintFromCtx (ssl);
      std::cout << "Could not verify SSL servers certificate (self signed)." << std::endl;
      std::cout << "Fingerprint is: "<< fingerprint << std::endl;
      std::cout << "Do you want to continue? [y|n]: ";
      std::cin >> answer;

      delete[] fingerprint;

      if (answer == 'y' || answer == 'Y')
        return TRUE;
      else
        return FALSE;
    }

  return TRUE;
}

void *Network::check_connections_queue (void *arg)
{

  int i;
  user_ls_t *user_ls;
  user_ls = (user_ls_t *) arg;

  if (user_ls->count == 0)
    return NULL;

  for (i = 0; i < user_ls->count; i++)

    /* check if we're connected to peer */
    if (!Peers::UserIsConnected (user_ls->user[i]))
      Network::Client::PeerConnect (user_ls->address[i], PORT);

  return NULL;
}
