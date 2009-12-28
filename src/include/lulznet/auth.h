/*
 * "auth.h" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

#ifndef _LNET_AUTH_H
#define _LNET_AUTH_H

#define PW_HASH_STRING_LEN      32

namespace Auth
{

/* Check if hash match username */
int DoAuthentication (std::string Username, uChar *Hash);

/* Ask for password (disable echo) */
void PasswordPrompt ();

std::string GetHash (std::string RequestedUser);

namespace Crypt
{

/* return string's md5 */
uChar *CalculateMd5 (std::string string);

/* return a string with ssl'peer certificate fingerprint */
char *GetFingerprintFromCtx (SSL * ssl);
}
}

#endif
