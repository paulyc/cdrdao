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
 * Revision 1.1.1.1  2000/02/05 01:35:13  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.5  1999/04/05 11:04:48  mueller
 * Added decoding of media catalog number and ISRC code.
 *
 * Revision 1.4  1999/03/27 20:58:55  mueller
 * Added various access functions.
 *
 * Revision 1.3  1998/09/07 15:20:20  mueller
 * Reorganized read-toc related code.
 *
 * Revision 1.2  1998/08/30 19:10:32  mueller
 * Added handling of Catalog Number and ISRC codes.
 *
 * Revision 1.1  1998/08/29 21:31:00  mueller
 * Initial revision
 *
 *
 */

#ifndef __SUB_CHANNEL_H__
#define __SUB_CHANNEL_H__

class SubChannel {
public:
  enum Type { QMODE1TOC,  // toc data
	      QMODE1DATA, // current position in data
	      QMODE2,     // Catalog number
	      QMODE3,     // ISRC code
	      QMODE5TOC,  // toc data 
	      QMODE_ILLEGAL // indicates illegal adr field
  };

  SubChannel();
  virtual ~SubChannel();

  // marks the CRC of this sub-channel as invalid, 'checkCrc()' will always
  // return true for such sub-channels
  void crcInvalid(); 

  // virtual constructors:
  // create sub channel with specified q-mode
  virtual SubChannel *makeSubChannel(Type) = 0;
  // create sub channel with reading sub channel data from given buffer
  virtual SubChannel *makeSubChannel(unsigned char *) = 0;

  virtual void type(unsigned char) = 0; // sets Q mode type
  virtual Type type() const; // return Q mode type

  virtual long dataLength() const = 0; // returns number of sub channel bytes

  virtual void pChannel(int) = 0; // sets P channel bit

  virtual void ctl(int) = 0;     // sets control flags
  virtual unsigned char ctl() const = 0; // return control nibbles in bits 0-3

  virtual void trackNr(int) = 0;   // sets track number (QMODE1DATA)
  virtual int trackNr() const = 0; // returns track number (QMODE1DATA)

  virtual void indexNr(int) = 0;   // sets index number (QMODE1DATA)
  virtual int indexNr() const = 0; // returns index number (QMODE1DATA)

  virtual void point(int) = 0;   // sets point filed (QMODE1TOC, QMODE5TOC)

  virtual void min(int) = 0;     // track relative time (QMODE1TOC, QMODE1DATA, QMODE5TOC)
  virtual int min() const = 0;

  virtual void sec(int) = 0;     // track relative time (QMODE1TOC, QMODE1DATA, QMODE5TOC)
  virtual int sec() const = 0;

  virtual void frame(int) = 0;   // track relative time (QMODE1TOC, QMODE1DATA, QMODE5TOC)
  virtual int frame() const = 0;

  virtual void amin(int) = 0;    // absolute time (QMODE1DATA)
  virtual int amin() const = 0;

  virtual void asec(int) = 0;    // absolute time (QMODE1DATA)
  virtual int asec() const = 0;

  virtual void aframe(int) = 0;  // absolute time (QMODE1DATA, QMODE2, QMODE3)
  virtual int aframe() const = 0;

  virtual void pmin(int) = 0;    // track start time (QMODE1TOC, QMODE5TOC)
  virtual void psec(int) = 0;    // track start time (QMODE1TOC, QMODE5TOC)
  virtual void pframe(int) = 0;  // track start time (QMODE1TOC, QMODE5TOC)

  virtual void zero(int) = 0; // zero field (QMODE5TOC)

  // set catalog number (QMODE2)
  virtual void catalog(char, char, char, char, char, char, char, char, char,
		       char, char, char, char) = 0;
  // return catalog number
  virtual const char *catalog() const = 0;

  // set ISRC code (QMODE3)
  virtual void isrc(char, char, char, char, char, char, char, char, char,
		    char, char, char) = 0;

  // returns ISRC code
  virtual const char *isrc() const = 0;

  virtual void print() const = 0;

  virtual void calcCrc() = 0; // calculates crc and stores it in crc fields
  virtual int checkCrc() const = 0;

  virtual int checkConsistency();

  virtual const unsigned char *data() const = 0;

  static unsigned char ascii2Isrc(char);
  static char isrc2Ascii(unsigned char);

  static unsigned char bcd(int);
  static int bcd2int(unsigned char d);
  static int isBcd(unsigned char);

protected:
  Type type_;
  int crcValid_; // 0 if sub channel has no valid CRC that can be checked,
                 // 1 if CRC is valid and can be checked
  static unsigned short crctab[256];

  static void encodeCatalogNumber(unsigned char *, char, char, char, char,
				  char, char, char, char, char, char, char,
				  char, char);
  static void decodeCatalogNumber(const unsigned char *, char *, char *,
				  char *, char *, char *, char *, char *,
				  char *, char *, char *, char *, char *,
				  char *);

  static void encodeIsrcCode(unsigned char *, char, char, char, char, char, 
			     char, char, char, char, char, char, char);
  static void decodeIsrcCode(const unsigned char *in, char *c1, char *c2,
			     char *o1, char *o2, char *o3, char *y1,
			     char *y2, char *s1, char *s2, char *s3,
			     char *s4, char *s5);

};

#endif
