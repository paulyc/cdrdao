/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998  Andreas Mueller <mueller@daneb.ping.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.7  1999/03/27 19:51:04  mueller
 * Added 'strdup3CC()'.
 *
 * Revision 1.6  1999/01/24 15:59:02  mueller
 * Moved 'audioDataLength()', 'audioFileType()' and 'waveHeaderLength()'
 * to class 'AudioData'.
 *
 * Revision 1.5  1998/10/03 14:32:31  mueller
 * Applied patch from Bjoern Fischer <bfischer@Techfak.Uni-Bielefeld.DE>.
 *
 * Revision 1.4  1998/09/06 12:00:26  mueller
 * Used 'message' function for messages.
 *
 * Revision 1.3  1998/08/15 12:41:32  mueller
 * Ignore case when checking for '.wav' extension.
 * Suggested by Paul Martin <pm@nowster.zetnet.co.uk>.
 *
 */

static char rcsid[] = "$Id: util.cc,v 1.1.1.1 2000-02-05 01:34:32 llanero Exp $";

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util.h"
#include "Sample.h"

char *strdupCC(const char *s)
{
  char *ret;
  long len;

  if (s == NULL) {
    return NULL;
  }

  len = strlen(s);

  ret = new char[len + 1];

  strcpy(ret, s);

  return ret;
}

char *strdup3CC(const char *s1, const char *s2, const char *s3)
{
  char *ret;
  long len = 0;

  if (s1 == NULL && s2 == NULL && s3 == NULL)
    return NULL;

  if (s1 != NULL)
    len = strlen(s1);

  if (s2 != NULL)
    len += strlen(s2);

  if (s3 != NULL)
    len += strlen(s3);

  ret = new char[len + 1];

  *ret = 0;

  if (s1 != NULL)
    strcpy(ret, s1);

  if (s2 != NULL)
    strcat(ret, s2);

  if (s3 != NULL)
    strcat(ret, s3);

  return ret;
}

long fullRead(int fd, void *buf, long count)
{
  long n = 0;
  long nread = 0;
  
  do {
    do {
      n = read(fd, (char *)buf + nread, count);
    } while (n < 0 && (errno == EAGAIN || errno == EINTR));

    if (n < 0) {
      return -1;
    }

    if (n == 0) {
      return nread;
    }
    
    count -= n;
    nread += n;
  } while (count > 0);

  return nread;
}

long fullWrite(int fd, const void *buf, long count)
{
  long n;
  long nwritten = 0;
  const char *p = (const char *)buf;

  do {
    do {
      n = write(fd, p, count);
    } while (n < 0 && (errno == EAGAIN || errno == EINTR));

    if (n < 0)
      return -1;

    if (n == 0)
      return nwritten;

    count -= n;
    nwritten += n;
    p += n;
  } while (count > 0);

  return nwritten;
}
  
long readLong(FILE *fp)
{
  unsigned char c1 = getc(fp);
  unsigned char c2 = getc(fp);
  unsigned char c3 = getc(fp);
  unsigned char c4 = getc(fp);

  return ((long)c4 << 24) | ((long)c3 << 16) | ((long)c2 << 8) | (long)c1;
}

short readShort(FILE *fp)
{
  unsigned char c1 = getc(fp);
  unsigned char c2 = getc(fp);

  return ((short)c2 << 8) | (short)c1;
}

void swapSamples(Sample *buf, unsigned long len)
{
  unsigned long i;

  for (i = 0; i < len; i++) {
    buf[i].swap();
  }
}

unsigned char int2bcd(int d)
{
  if (d >= 0 && d <= 99)
    return ((d / 10) << 4) | (d % 10);
  else 
    return d;
}

int bcd2int(unsigned char d)
{
  unsigned char d1 = d & 0x0f;
  unsigned char d2 = d >> 4;

  if (d1 <= 9 && d2 <= 9) {
    return d2 * 10 + d1;
  }
  else {
    return d;
  }
}

const char *stripCwd(const char *fname)
{
  static char *buf = NULL;
  static long bufLen = 0;

  char cwd[PATH_MAX + 1];
  long len;
  
  if (fname == NULL)
    return NULL;

  len = strlen(fname);

  if (buf == NULL || len >= bufLen) {
    bufLen = len + 1;
    delete[] buf;
    buf = new char[bufLen];
  }

  if (getcwd(cwd, PATH_MAX + 1) == NULL) {
    // if we cannot retrieve the current working directory return 'fname'
    strcpy(buf, fname);
  }
  else {
    len = strlen(cwd);

    if (strncmp(cwd, fname, len) == 0) {
      if (*(fname + len) == '/')
	strcpy(buf, fname + len + 1);
      else
	strcpy(buf, fname + len);

      if (buf[0] == 0) {
	// resulting filename would be "" -> return 'fname'
	strcpy(buf, fname);
      }
    }
    else {
      strcpy(buf, fname);
    }
  }

  return buf;
}
