/*
 * "log.c" (C) blawl ( j[dot]segf4ult[at]gmail[dot]com )
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

pthread_mutex_t Log::mutex;

void Log::Info (const char *msg, ...)
{
  va_list args;

  va_start (args, msg);
  DoLog (msg, args, INFO);
  va_end (args);
}

#ifdef DEBUG
void Log::Debug1 (const char *msg, ...)
{
  va_list args;

  if (options.debugLevel () < 1)
    return;

  va_start (args, msg);
  DoLog (msg, args, DEBUG_1);
  va_end (args);
}

void Log::Debug2 (const char *msg, ...)
{
  va_list args;

  if (options.debugLevel () < 2)
    return;

  va_start (args, msg);
  DoLog (msg, args, DEBUG_2);
  va_end (args);
}

void Log::Debug3 (const char *msg, ...)
{
  va_list args;

  if (options.debugLevel () < 3)
    return;

  va_start (args, msg);
  DoLog (msg, args, DEBUG_3);
  va_end (args);
}
#endif

void Log::Error (const char *msg, ...)
{
  va_list args;

  va_start (args, msg);
  DoLog (msg, args, ERROR);
  va_end (args);
}

void Log::Fatal (const char *msg, ...)
{
  va_list args;

  va_start (args, msg);
  DoLog (msg, args, FATAL);
  va_end (args);

  LulznetExit ();

  exit (1);
}

void Log::DoLog (const char *fmt, va_list args, int level)
{
  char msgbuf[MAXLOGSIZE];
  char fmtbuf[MAXLOGSIZE];
  pthread_mutex_lock (&mutex);

  switch (level)
    {
    case INFO:
      snprintf (fmtbuf, sizeof (fmtbuf), "[inf] %s\n", fmt);
      break;
#ifdef DEBUG
    case DEBUG_1:
    case DEBUG_2:
    case DEBUG_3:
      snprintf (fmtbuf, sizeof (fmtbuf), "[dbg] %s\n", fmt);
      break;
#endif
    case ERROR:
      snprintf (fmtbuf, sizeof (fmtbuf), "[err] %s\n", fmt);
      break;
    case FATAL:
      snprintf (fmtbuf, sizeof (fmtbuf), "[ftl] %s\n", fmt);
      break;
    }

  vsnprintf (msgbuf, sizeof (msgbuf), fmtbuf, args);
  fprintf (stderr, "%s", msgbuf);
  fflush (stderr);

  /* if specified log file print to it
     if(opt.log_fp != NULL)
     fprintf(opt.log_fp,"%s",msgbuf");
   */

  pthread_mutex_unlock (&mutex);

}

#ifdef DEBUG
void Log::Dump (unsigned char *data_buffer, int length)
{
  char byte;
  int i;
  int j;

  if (options.debugLevel () < 4)
    return;

  pthread_mutex_lock (&mutex);

  for (i = 0; i < length; i++)
    {
      byte = data_buffer[i];
      fprintf (stderr, "%02x ", data_buffer[i]);
      if (((i % 32) == 31) || (i == length - 1))
        {
          for (j = 0; j < 31 - (i % 32); j++)
            fprintf (stderr, "   ");
          fprintf (stderr, "| ");
          for (j = (i - (i % 32)); j <= i; j++)
            {
              byte = data_buffer[j];
              if ((byte > 31) && (byte < 127))
                fprintf (stderr, "%c", byte);
              else
                fprintf (stderr, ".");
            }
          fprintf (stderr, "\n");
        }

    }

  pthread_mutex_unlock (&mutex);
}
#endif
