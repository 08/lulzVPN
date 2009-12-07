/*
 * "auth.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

int
Auth::DoAuthentication (std::string username, u_char * hash)
{
  std::string strHash;
  std::string localHash;
  char tmp[3];
  int response;
  int i;

  response = FALSE;

  localHash = File::get_hash (username);
  if (!localHash.empty ())
    {
      for (i = 0; i < MD5_DIGEST_LENGTH; i++)
        {
          sprintf (tmp, "%02x", hash[i]);
          strHash.append (tmp);
        }
      if (!strHash.compare (localHash))
        response = TRUE;
      else
        Log::Error ("Wrong password");
    }
  else
    Log::Error ("Cannot find user");

  return response;
}

void
Auth::PasswordPrompt ()
{

  std::string password;
  struct termio tty, oldtty;

  ioctl (0, TCGETA, &oldtty);

  tty = oldtty;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 0;

  ioctl (0, TCSETA, &tty);

  std::cout << "Password: ";
  std::cin >> password;
  std::cout << std::endl;

  ioctl (0, TCSETA, &oldtty);

  options.password(password);
}

int
Auth::File::GetUserCredentials (FILE * fp, std::string * username, std::string * hash)
{
  char tmp[50];
  int i;

  if (fscanf (fp, "%49s", tmp) == -1)
    return 0;

  for (i = 0; tmp[i] != ':' && i < MAX_USERNAME_LEN; i++)
    username->append (1, tmp[i]);

  hash->assign (tmp + i + 1, PW_HASH_STRING_LEN);
  return 1;
}

std::string
Auth::File::get_hash (std::string request_user)
{

  FILE *cred;
  std::string user;
  std::string hash;

  cred = fopen (CREDENTIAL_FILE, "r");

  if (cred == NULL)
    Log::Error ("Cannot open credential file %s", CREDENTIAL_FILE);
  else
    while (GetUserCredentials (cred, &user, &hash))
      if (!user.compare (request_user))
        return hash;
      else
        user.clear ();

  hash.clear();
  return hash;
}

u_char *
Auth::Crypt::CalculateMd5 (std::string string)
{
  EVP_MD_CTX mdctx;
  const EVP_MD *md;
  u_int md_len;
  u_char *hexHash;

  hexHash = new u_char[MD5_DIGEST_LENGTH];

  md = EVP_get_digestbyname ("MD5");
  EVP_MD_CTX_init (&mdctx);
  EVP_DigestInit_ex (&mdctx, md, NULL);
  EVP_DigestUpdate (&mdctx, string.c_str (), string.length ());
  EVP_DigestFinal_ex (&mdctx, hexHash, &md_len);
  EVP_MD_CTX_cleanup (&mdctx);

  return hexHash;
}

char *
Auth::Crypt::GetFingerprintFromCtx (SSL * ssl)
{
  u_char digest[SHA_DIGEST_LENGTH];
  char hex[] = "0123456789ABCDEF";
  char *fp = new char[EVP_MAX_MD_SIZE * 3];
  u_int len;
  u_int i;
  X509 *cert;

  cert = SSL_get_peer_certificate (ssl);
  X509_digest (cert, EVP_md5 (), digest, &len);

  for (i = 0; i < len; i++)
    {
      fp[i * 3 + 0] = hex[(digest[i] >> 4) & 0xF];
      fp[i * 3 + 1] = hex[(digest[i] >> 0) & 0xF];
      fp[i * 3 + 2] = i == len - 1 ? '\0' : ':';
    }

  return fp;
}
