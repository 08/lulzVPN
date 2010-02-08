/*
 * "peer.cpp" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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
#include <lulznet/log.h>
#include <lulznet/packet.h>
#include <lulznet/networking.h>

Packet::Packet *
Packet::BuildDisassociationPacket () 
{
  Packet *packet;

  packet = new Packet;
  packet->buffer[0] = controlPacket;
  packet->buffer[1] = closeConnection;

  packet->length = 2;

  return packet;
}

Packet::Packet *
Packet::BuildNewPeerNotifyPacket (std::string user, int address)
{
  Packet *packet;

  packet = new Packet;
  packet->buffer[0] = controlPacket;
  packet->buffer[1] = newPeerNotify;

  sprintf((char *) packet->buffer + 2, "%s ", user.c_str()); 
  memcpy(packet->buffer + 2 + user.length() + 1, (char *) &address , 4);

  packet->length = 2 + user.length() + 1 + 4;

  return packet;

}

uInt
Packet::GetDestinationIp (Packet::Packet * packet)
{
  eth_header *ethHdr;
  arp_header *arpHdr;
  ip_header *ipHdr;
  uInt address;
  u_short protocol;

#ifdef DEBUG
  char p_addr[addressLenght];
#endif

  ethHdr = (eth_header *)(packet->buffer + 2);
  protocol = ethHdr->eth_type;
  protocol = ntohs(protocol);

  if (protocol == 0x0806) {
    /*arp packet */

    arpHdr = (arp_header *)(packet->buffer + ETH_HDR_LEN + 2);
    address = arpHdr->dst_ip_adr;

#ifdef DEBUG
    inet_ntop(AF_INET, &address, p_addr, addressLenght);
    Log::Debug3("\tarp packet, dst: %s", p_addr);
#endif

  }
  else if (protocol == 0x0800) {
    /* ip packet */

    ipHdr = (ip_header *)(packet->buffer + ETH_HDR_LEN + 2);
    address = ipHdr->dst_adr;

#ifdef DEBUG
    inet_ntop(AF_INET, &address, p_addr, addressLenght);
    Log::Debug3("\tip packet, dst: %s", p_addr);
#endif
  }
  else
    /*TODO: add more protocol, for now
       we assume it's a faggot packet
       and we return 0 */
    address = 0;

  return address;
}
