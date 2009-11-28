/*
 * "peer.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

u_int PacketInspection::get_destination_ip (Network::Packet *packet)
{
  eth_header *eth_hdr;
  arp_header *arp_hdr;
  ip_header *ip_hdr;
  u_int address;
  u_short protocol;

#ifdef DEBUG
  char p_addr[ADDRESS_LEN];
#endif

  eth_hdr = (eth_header *) (packet->buffer + 1);
  protocol = eth_hdr->eth_type;
  protocol = ntohs (protocol);

  if (protocol == 0x0806)
    {
      /*arp packet */

      arp_hdr = (arp_header *) (packet->buffer + ETH_HDR_LEN + 1);
      address = arp_hdr->dst_ip_adr;
#ifdef DEBUG
      inet_ntop (AF_INET, &address, p_addr, ADDRESS_LEN);
      Log::Debug3 ("\tarp packet, dst: %s", p_addr);
#endif

    }
  else if (protocol == 0x0800)
    {
      /* ip packet */

      ip_hdr = (ip_header *) (packet->buffer + ETH_HDR_LEN + 1);
      address = ip_hdr->dst_adr;

#ifdef DEBUG
      inet_ntop (AF_INET, &address, p_addr, ADDRESS_LEN);
      Log::Debug3 ("\tip packet, dst: %s", p_addr);
#endif
    }
  else
    /*TODO: add more protocol, for now
       we assume it's a faggot packet
       and we return 0 */
    address = 0;

  return address;
}

