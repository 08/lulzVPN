/*
 * "config.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

#include <lulznet/config.h>
#include <lulznet/log.h>
#include <lulznet/networking.h>

Config::Config ()
{
  _Flags = 0;
  _ConnectingPort = PORT;
  _BindingPort = PORT;
#ifdef DEBUG
  _DebugLevel = 0;

  _TapDevicesCount = 0;
  _UserCredentialsCount = 0;

#endif
}

int Config::Flags ()
{
  return _Flags;
}

short Config::ConnectingPort ()
{
  return _ConnectingPort;
}

short Config::BindingPort ()
{
  return _BindingPort;
}

std::string Config::ConnectingAddress ()
{
  return _ConnectingAddress;
}

std::string Config::BindingAddress ()
{
  return _BindingAddress;
}

std::string Config::Username ()
{
  return _Username;
}

std::string Config::Password ()
{
  return _Password;
}

void Config::Password (std::string password)
{
  _Password = password;
}

#ifdef DEBUG
int Config::DebugLevel ()
{
  return _DebugLevel;
}
#endif

int
Config::TapDevicesCount()
{
  return _TapDevicesCount;
}

TapDeviceT
Config::TapDevice(int i)
{
  return _TapDevices[i];
}

int
Config::UserCredentialsCount()
{
  return _UserCredentialsCount;
}

UserCredentialT
Config::UserCredentials(int i)
{
  return _UserCredentials[i];
}


void Config::ParseArgs (int argc, char **argv)
{
  int c;
  char optopt = '\x00';
  opterr = 0;

  while ((c = getopt (argc, argv, "ac:dhil:n:p:P:t:v")) != -1)
    switch (c)
      {
      case 'c':
        if (!*optarg)
          Log::Fatal ("You must specify an address");
        else
          _ConnectingAddress = optarg;
        break;
      case 'd':
        _Flags ^= LISTENING_MODE;
        break;
      case 'h':
        help ();
        break;
      case 'i':
        _Flags |= INTERACTIVE_MODE;
        break;
      case 'l':
        _Username = optarg;
        break;
      case 'p':
        if (!*optarg)
          Log::Fatal ("You must specify a port");
        else
          _ConnectingPort = (short) atoi (optarg);
        break;
      case 'P':
        if (!*optarg)
          Log::Fatal ("You must specify a port");
        else
          _BindingPort = (short) atoi (optarg);
        break;
#ifdef DEBUG
      case 'v':
        _DebugLevel++;
        break;
#endif
      case '?':
        if (optopt == 'p' || optopt == 'c')
          Log::Fatal ("Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          Log::Fatal ("Unknown option `-%c'.\n", optopt);
        else
          Log::Fatal ("Unknown option character `\\x%x'.\n", optopt);
      }
}

void Config::ParseConfigFile (char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr curNode;

  doc = xmlParseFile (filename);

  if (doc == NULL)
    {
      Log::Error ("Document not parsed successfully.");
      return;
    }

  curNode = xmlDocGetRootElement (doc);
  if (curNode == NULL)
    {
      Log::Fatal ("Empty config file");
      xmlFreeDoc (doc);
      return;
    }

  if (xmlStrcmp (curNode->name, (const xmlChar *) "lulzNetConfig"))
    {
      Log::Fatal ("This is not a valid lulznet config file.\nRoot node != lulzNetConfig");
      xmlFreeDoc (doc);
      return;
    }

  curNode = curNode->xmlChildrenNode;
  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "config")))
        ParseConfig (doc, curNode);
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "users")))
        ParseUsers (doc, curNode);
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "taps")))
        ParseTaps (doc, curNode);

      curNode = curNode->next;
    }

  xmlFreeDoc (doc);
  return;

}

void Config::ChecEmptyConfigEntry ()
{
  if (!_TapDevicesCount)
    Log::Fatal ("You must specify a tap address");

  if (_Username.empty ())
    Log::Fatal ("You must specify an username");
}

void
Config::ParseConfig (xmlDocPtr doc, xmlNodePtr curNode)
{
  xmlChar *key;
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "username")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _Username = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "password")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _Password = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "listening")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          if (!strcmp((char *) key, "yes"))
            _Flags |= LISTENING_MODE;
          else if (!strcmp ((char *) key, "no"))
            {
              if (_Flags & LISTENING_MODE)
                _Flags ^= LISTENING_MODE;
            }
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "interactive")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          if (!strcmp((char *) key, "yes"))
            _Flags |= INTERACTIVE_MODE;
          else if (!strcmp ((char *) key, "no"))
            {
              if (_Flags & INTERACTIVE_MODE)
                _Flags ^= INTERACTIVE_MODE;
            }
          xmlFree (key);
        }
#ifdef DEBUG
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "debug")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _DebugLevel = atoi ((char *) key);
          xmlFree (key);
        }
#endif
      else
        Log::Error ("Invalid option in lulznet config");

      curNode = xmlNextElementSibling (curNode);
    }
  return;
}

void
Config::ParseUserNet (xmlDocPtr doc, xmlNodePtr curNode)
{
  xmlChar *key;
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "name")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          xmlFree (key);
        }
      else
        Log::Error ("Invalid option in allowed net config");

      curNode = xmlNextElementSibling (curNode);
    }
  return;
}

void
Config::ParseUser (xmlDocPtr doc, xmlNodePtr curNode)
{
  xmlChar *key;
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "name")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _UserCredentials[_UserCredentialsCount].Name = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "hash")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _UserCredentials[_UserCredentialsCount].Hash = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "allowedTap")))
        ParseUserNet(doc,curNode);
      else
        Log::Error ("Invalid option in user config");

      curNode = xmlNextElementSibling (curNode);
    }

  if (!(_UserCredentials[_UserCredentialsCount].Name.empty() ||
        _UserCredentials[_UserCredentialsCount].Hash.empty()))
    _UserCredentialsCount++;

  return;
}

void
Config::ParseUsers (xmlDocPtr doc, xmlNodePtr curNode)
{
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "user")))
        ParseUser(doc, curNode);
      else
        Log::Error ("Invalid option in users config");

      curNode = xmlNextElementSibling (curNode);
    }
  return;
}

void
Config::ParseTap (xmlDocPtr doc, xmlNodePtr curNode)
{
  xmlChar *key;
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "name")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 2);
          _TapDevices[_TapDevicesCount].NetworkName = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "address")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _TapDevices[_TapDevicesCount].Address = (char *) key;
          xmlFree (key);
        }
      else if ((!xmlStrcmp (curNode->name, (const xmlChar *) "netmask")))
        {
          key = xmlNodeListGetString (doc, curNode->xmlChildrenNode, 1);
          _TapDevices[_TapDevicesCount].Netmask = (char *) key;
          xmlFree (key);
        }
      else
        Log::Error ("Invalid option in tap config");

      curNode = xmlNextElementSibling (curNode);
    }

  if (!(_TapDevices[_TapDevicesCount].NetworkName.empty() ||
        _TapDevices[_TapDevicesCount].Address.empty() ||
        _TapDevices[_TapDevicesCount].Netmask.empty()))
    _TapDevicesCount++;

  return;

}

void
Config::ParseTaps (xmlDocPtr doc, xmlNodePtr curNode)
{
  curNode = xmlFirstElementChild (curNode);

  while (curNode != NULL)
    {
      if ((!xmlStrcmp (curNode->name, (const xmlChar *) "tap")))
        ParseTap(doc,curNode);
      else
        Log::Error ("Invalid option in taps config");

      curNode = xmlNextElementSibling (curNode);
    }
  return;
}

