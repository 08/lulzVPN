/*
 * "select.h" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
 *
 * lulzVPN is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * lulzVPN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _LVPN_SELECT_H
#define _LVPN_SELECT_H

extern int udpDataSock;

namespace Select
{

namespace TapChannel {
extern fd_set Set;
extern pthread_t ThreadId;

void *Loop (void __attribute__ ((unused)) *arg);
void Restart();
}

namespace DataChannel {
namespace Server {
extern pthread_t ThreadId;

void *Loop (void __attribute__ ((unused)) *arg);
}

namespace Client {
extern fd_set Set;
extern pthread_t ThreadId;

void *Loop (void __attribute__ ((unused)) *arg);
void Restart();
}
}

namespace CtrlChannel {
extern fd_set Set;
extern pthread_t ThreadId;

void *Loop (void __attribute__ ((unused)) *arg);
void Restart();
}
}

#endif

