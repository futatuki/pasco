/*
 * Copyright (C) 2003, by Keith J. Jones.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <sys/types.h>
#include <sys/uio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
# if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) \
     || defined(__DragonFly__)
#  include <sys/endian.h> 
# elif defined(__linux__) || defined(__CYGWIN__)
#  include <endian.h> 
# elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define le32toh(x) OSSwapLittleToHostInt32(x)
#  define le64toh(x) OSSwapLittleToHostInt64(x)
# endif

const char *revision = "$Id$";
const unsigned int version_major = 1;
const unsigned int version_minor = 1;

//
/* This is the default block size for an activity record */
//
#define BLOCK_SIZE	0x80

#ifdef CYGWIN
ssize_t pread( int d, void *buf, size_t nbytes, off_t offset) {
  lseek( d, offset, SEEK_SET );
  read( d, buf, nbytes );
}
#endif

//
/* uint64_t Windows NT time stamp to time_t (and timespec) Unix Timestamp */
//
time_t win_time_to_unix( uint64_t val , struct timespec *ts) {
  long nsec;
  time_t sec;

  if (val == 0) {
    sec = 0; 
    if (ts != NULL) {
      ts->tv_sec = 0;
      ts->tv_nsec = 0;
    }
  }
  else {
    sec  = (val / 10000000L) - 11644473600;
    if (ts != NULL) {
      nsec = (val % 10000000L) * 100;
      ts->tv_sec  = sec;
      ts->tv_nsec = nsec;
    }
  }
  return sec;
}

//
/* timespec to ISO format timestamp string */
//
void timespec_to_isoformat(const struct timespec *ts, char *isostr){
  struct tm *tm_p;
  int toff;
  char toffsign;
  int toffh, toffm;
  char toffstr[7];

  tm_p = localtime(&(ts->tv_sec)); 
  if (tm_p->tm_gmtoff != 0) {
    if (tm_p->tm_gmtoff > 0) {
      toffsign = '+';
      toff = tm_p->tm_gmtoff / 60; 
    }
    else {
      toffsign = '-';
      toff = - tm_p->tm_gmtoff / 60; 
    }
    toffh = toff / 60;
    toffm = toff % 60;
    snprintf(toffstr, 7, "%c%02u:%02u",
        toffsign, (unsigned int)toffh, (unsigned int)toffm);
  }
  else {
    toffstr[0] = 'Z', toffstr[1] = '\0';
  }
  if (ts->tv_nsec  == 0) {
    snprintf(isostr, 26, "%04u-%02u-%02uT%02u:%02u:%02u%s",
        (unsigned int) (tm_p->tm_year + 1900),
        (unsigned int) tm_p->tm_mon,
        (unsigned int) tm_p->tm_mday,
        (unsigned int) tm_p->tm_hour,
        (unsigned int) tm_p->tm_min,
        (unsigned int) tm_p->tm_sec,
        toffstr);
  }
  else {
    snprintf(isostr, 36, "%04u-%02u-%02uT%02u:%02u:%02u.%09lu%s",
        (unsigned int) (tm_p->tm_year + 1900),
        (unsigned int) tm_p->tm_mon,
        (unsigned int) tm_p->tm_mday,
        (unsigned int) tm_p->tm_hour,
        (unsigned int) tm_p->tm_min,
        (unsigned int) tm_p->tm_sec,
        (unsigned long) (ts->tv_nsec),
        toffstr);
  }
  return;
}

//
/* This function prepares a string for nice output */
//
int printablestring( char *str ) {
  int i;

  i = 0;
  while ( str[i] != '\0' ) {
    if ( (unsigned char)str[i] < 32 || (unsigned char)str[i] > 127 ) {
      str[i] = ' ';
    }
    i++; 
  }
  return 0;
}

//
/* This function parses a REDR record. */
//
void parse_redr( int history_file, off_t currrecoff, const char *delim, size_t filesize, char *type ) {
  uint32_t fourbytes;
  int i;
  size_t reclen;
  off_t redroff;
  char *url;
  char *filename;
  char *httpheaders;
  char ascmodtime[26], ascaccesstime[26];
  char dirname[9];
  char *cp;


  pread( history_file, &fourbytes, 4, currrecoff+4 );
  reclen = le32toh(fourbytes) * BLOCK_SIZE;

  url = (char *)malloc( reclen+1 );

  for (redroff = currrecoff + 0x10, cp = url, i = 0;
        redroff < filesize && i < reclen ; redroff++, cp++, i++) {
    pread( history_file, cp, 1, redroff );
    if ( *cp == '\0' ) break;
  }
  *cp = '\0';
  if (i == reclen) {
      fprintf(stderr,
          "warn: corrupted data or unknown structure in parse_redr: "
          "offset: 0x%llx\n", (long long)currrecoff);
  }


  filename = (char *)malloc( 1 );
  filename[0] = '\0';

  httpheaders = (char *)malloc( 1 );
  httpheaders[0] = '\0';

  dirname[0] = '\0';

  ascmodtime[0] = '\0';
  ascaccesstime[0] = '\0';
  dirname[0] = '\0';

  printablestring( type );
  printablestring( url );
  printablestring( ascmodtime );
  printablestring( ascaccesstime );
  printablestring( filename );
  printablestring( dirname );
  printablestring( httpheaders );
  printf( "%s%s%s%s%s%s%s%s%s%s%s%s%s\n", type, delim, url, delim, ascmodtime, delim, ascaccesstime, delim, filename, delim, dirname, delim, httpheaders );

  type[0] = '\0';

  free( url );
  free( filename );
  free( httpheaders );
  return;
}

//
/* This function parses a URL and LEAK activity record. */
//
void parse_url( int history_file, off_t currrecoff, char *delim, size_t filesize, char *type , int isofmt) {
  uint32_t fourbytes;
  uint64_t eightbytes;
  char chr;
  off_t filenameoff;
  off_t httpheadersoff;
  off_t urloff;
  int i;
  size_t reclen;
  off_t dirnameoff;
  time_t modtime;
  time_t accesstime;
  struct timespec ts_modtime;
  struct timespec ts_accesstime;
  char ascmodtime[36], ascaccesstime[36];
  char dirname[9];
  char *url;
  char *filename;
  char *httpheaders;
  char *cp;


  pread( history_file, &fourbytes, 4, currrecoff+4 );
  reclen = le32toh(fourbytes) * BLOCK_SIZE;

  pread( history_file, &eightbytes, 8, currrecoff+8 );
  modtime = win_time_to_unix( le64toh(eightbytes), &ts_modtime );
  
  pread( history_file, &eightbytes, 8, currrecoff+16 );
  accesstime = win_time_to_unix( le64toh(eightbytes), &ts_accesstime );
 
  if (accesstime == 0) {
    ascaccesstime[0] = '\0';
  }
  else {
    if (isofmt) {
      timespec_to_isoformat(&ts_accesstime, ascaccesstime );
    }
    else {
      ctime_r( &accesstime, ascaccesstime );
    }
  }

  if (modtime == 0) {
    ascmodtime[0] = '\0';
  }
  else {
    if (isofmt) {
      timespec_to_isoformat( &ts_modtime, ascmodtime );
    }
    else {
      ctime_r( &modtime, ascmodtime );
    }
  }
  url = (char *)malloc( reclen+1 );

  pread( history_file, &chr, 1, currrecoff+0x34 );
  for (urloff = currrecoff + (unsigned char)chr, cp = url, i = 0;
        urloff < filesize && i < reclen ; urloff++, cp++, i++) {
    pread( history_file, cp, 1, urloff );
    if ( *cp == '\0' ) break;
  }
  *cp = '\0';
  if (i == reclen) {
      fprintf(stderr,
          "warn: corrupted data or unknown structure in parse_url, url field: "
          "offset: 0x%llx\n", (long long)currrecoff);
  }

  filename = (char *)malloc( reclen+1 );

  pread( history_file, &fourbytes, 4, currrecoff+0x3C );
  filenameoff = le32toh(fourbytes) + currrecoff; 
  for ( filenameoff = currrecoff + le32toh(fourbytes),
            cp = filename, i = 0;
        filenameoff < filesize && i < reclen ;
        filenameoff++, cp++, i++) {
    pread( history_file, cp, 1, filenameoff );
    if ( *cp == '\0' ) break;
  }
  *cp = '\0';
  if (i == reclen) {
      fprintf(stderr,
          "warn: corrupted data or unknown structure in parse_url, filename field: "
          "offset: 0x%llx\n", (long long)currrecoff);
  }


  pread( history_file, &chr, 1, currrecoff+0x39 );
  dirnameoff = (unsigned char)chr;

  if (0x50+(12*dirnameoff)+8 < filesize) {
    pread( history_file, dirname, 8, 0x50+(12*dirnameoff) );
    dirname[8] = '\0';
  } else {
    dirname[0] = '\0';
  }

  httpheaders = (char *)malloc( reclen+1 );

  pread( history_file, &fourbytes, 4, currrecoff+0x44 );
  for ( httpheadersoff = currrecoff + le32toh(fourbytes),
            cp = httpheaders, i = 0;
        httpheadersoff < filesize && i < reclen ;
        httpheadersoff++, cp++, i++) {
    pread( history_file, cp, 1, httpheadersoff );
    if ( *cp == '\0' ) break;
  }
  *cp = '\0';
  if (i == reclen) {
      fprintf(stderr,
          "warn: corrupted data or unknown structure in parse_url, httpheaders field: "
          "offset: 0x%llx\n", (long long)currrecoff);
  }

  printablestring( type );
  printablestring( url );
  printablestring( ascmodtime );
  printablestring( ascaccesstime );
  printablestring( filename );
  printablestring( dirname );
  printablestring( httpheaders );

  if (type[3] == ' ') {
    type[3] = '\0';
  }

  printf( "%s%s%s%s%s%s%s%s%s%s%s%s%s\n", type, delim, url, delim, ascmodtime, delim, ascaccesstime, delim, filename, delim, dirname, delim, httpheaders );

  type[0] = '\0';
  dirname[0] = '\0';
  ascmodtime[0] = '\0';
  ascaccesstime[0] = '\0';

  free( url );
  free( filename );
  free( httpheaders );
  return;
}

void parse_unknown( int history_file, off_t currrecoff, char *delim, size_t filesize, char *type ) {
  type[0] = '\0'; 
  return;
}

//
/* This function prints the usage message */
//
void usage( void ) {
  printf("\nUsage:  pasco [options] <filename>\n" );
  printf("\t-d Undelete Activity Records\n" );
  printf("\t-t Field Delimiter (TAB by default)\n" );
  printf("\t-i Use ISO 8601 format for time stamp\n" );
  printf("\t-V print version and then exit\n" );
  printf("\n\n");
  return;
}

void printversion( void ) {
  printf("pasco ver. %u.%02u\n%s\n", version_major, version_minor, revision);
  return;
}


//
/* MAIN function */
//
int main( int argc, char **argv ) {
  int history_file;
  uint32_t fourbytes;
  char delim[10];
  off_t currrecoff;
  size_t filesize;
  int opt;
  char type[5];
  char hashrecflagsstr[4];
  off_t hashoff;
  size_t hashsize;
  off_t nexthashoff;
  off_t offset;
  // int hashrecflags;
  int deleted = 0;
  int isofmt = 0;


  if (argc < 2) {
    usage();
    exit( -2 );
  }

  while ((opt = getopt( argc, argv, "diVt:f:")) != -1) {
    switch(opt) {
      case 't':
        strncpy( delim, optarg, 10 );
        break;

      case 'd':
        deleted = 1;
        break;

      case 'i':
        isofmt = 1;
        break;

      case 'V':
        printversion();
        exit(0);
        break;

      default:
        usage();
        exit(-1);
    }
  }


  strcpy( delim, "\t" );

  printf("History File: %s\n\n", argv[argc-1]);
  history_file = open( argv[argc-1], O_RDONLY, 0 );

  if ( history_file <= 0 ) {
    printf("ERROR - The index.dat file cannot be opened!\n\n");
    usage();
    exit( -3 ); 
  }

  pread( history_file, &fourbytes, 4, 0x1C );
  filesize = le32toh(fourbytes);

  printf( "TYPE%sURL%sMODIFIED TIME%sACCESS TIME%sFILENAME%sDIRECTORY%sHTTP HEADERS\n", delim, delim, delim, delim, delim, delim );


  if (deleted == 0) {

    pread( history_file, &fourbytes, 4, 0x20 );
    hashoff = le32toh(fourbytes);
  
    while (hashoff != 0 ) {

      pread( history_file, &fourbytes, 4, hashoff+8 );
      nexthashoff = le32toh(fourbytes);

      pread( history_file, &fourbytes, 4, hashoff+4 );
      hashsize = le32toh(fourbytes) * BLOCK_SIZE;

      for (offset = hashoff + 16; offset < hashoff+hashsize; offset = offset+8) {
        pread( history_file, hashrecflagsstr, 4, offset );

        pread( history_file, &fourbytes, 4, offset+4 );
        currrecoff = le32toh(fourbytes);

        if (hashrecflagsstr[0] != 0x03 && currrecoff != 0xBADF00D ) {
          if (currrecoff != 0) {

            pread( history_file, type, 4, currrecoff );
            type[4] = '\0';

            if (type[0] == 'R' && type[1] == 'E' && type[2] == 'D' && type[3] == 'R' ) {

              parse_redr( history_file, currrecoff, delim, filesize, type );

            } else if ( (type[0] == 'U' && type[1] == 'R' && type[2] == 'L') || (type[0] == 'L' && type[1] == 'E' && type[2] == 'A' && type[3] == 'K') ) {

              parse_url( history_file, currrecoff, delim, filesize, type , isofmt);

            } else {

              parse_unknown( history_file, currrecoff, delim, filesize, type );

            }
          }
        }
      }  
    hashoff = nexthashoff;
    }
  } else if (deleted == 1) {

    currrecoff = 0;

    while (currrecoff < filesize ) {

      pread( history_file, type, 4, currrecoff );
      type[4] = '\0';

      if (type[0] == 'R' && type[1] == 'E' && type[2] == 'D' && type[3] == 'R' ) {

        parse_redr( history_file, currrecoff, delim, filesize, type );

      } else if ( (type[0] == 'U' && type[1] == 'R' && type[2] == 'L') || (type[0] == 'L' && type[1] == 'E' && type[2] == 'A' && type[3] == 'K') ) {

        parse_url( history_file, currrecoff, delim, filesize, type, isofmt);

      } else {

        parse_unknown( history_file, currrecoff, delim, filesize, type );

      }

      currrecoff = currrecoff + BLOCK_SIZE;
    }

  }
  close (history_file);
  return 0;
}
