/*
 * Fefe's fortune(6).  Public domain.
 * Developed under the One True OS, Linux.
 * Will output a random fortune to stdout.  Does not need strfile(1).
 * Will utilize system's /dev/urandom if available.
 * Uses mmap(2) for ease and performance.
 *
 * Usage: fortune <filename1> [<filename2>...]
 */

/* Added RE matching capability, getopt handling.
 * David Frey, Oct 1998
 */

/* [20010625] rip stdio and locale crap */

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* #include <stdio.h> */
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* #include <locale.h> */
#include <regex.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>

void carp(const char* msg,const char* filename) {
  const char* err=strerror(errno);
  write(2,msg,strlen(msg));
  if (filename) {
    write(2," `",2);
    write(2,filename,strlen(filename));
    write(2,"': ",3);
    write(2,err,strlen(err));
  }
  write(2,".\n",2);
}

void die(const char* msg,const char* filename) {
  carp(msg,filename);
  exit(1);
}

#ifndef _POSIX_MAPPED_FILES
#warning Your system headers say that mmap is not supported!
#endif

#define VERSION "1.0"
#define FORTUNEDIR "/usr/share/games/fortunes"

struct option const long_options[] =
{
  {"help",     no_argument,       0, 'h'},
  {"help",     no_argument,       0, '?'},
  {"match",    required_argument, 0, 'm'},
  {"version",  no_argument,       0, 'V'},
  {(char *)0,  0,                 0, (char)0}
};

const char *progname;

char *printquote(char *map, char *c, size_t len)
{
  const char* start;
  while (c > map &&
	 (*c != '%' || (c<map+len && *(c+1) != '\n') || *(c-1) != '\n'))
    c--;
  if (*c == '%')  c++;
  if (*c == '\n') c++;
  start=c;
  while (c < map + len &&
	 (*c != '%' || (c<map+len && *(c+1) != '\n') || *(c-1) != '\n'))
    c++;
  write(1,start,c-start);

  return c;
}

#if 0
char *regerr(regex_t *preg, int errcode)
{
  size_t errlen;
  char *errbuf;

  errlen=regerror(errcode, preg, NULL, 0);
  errbuf=(char *)malloc(errlen+1);
  if (errbuf == 0) {
    return NULL;
  } else {
    regerror(errcode, preg, errbuf, sizeof(errbuf));
    return errbuf;
  }
}
#endif

int printormatchfortune(char *filename, char *re)
     /* Search FILENAME for a given RE (if != NULL) or print out a
	random fortune (if RE == NULL)
      */
{
  int fd, len;
  int retcode = 1;
  char *map;

  fd = open(filename, 0);
  if (fd >= 0) {
    len = lseek(fd, 0, SEEK_END);
    map = (char *)mmap(0, len, PROT_READ, MAP_SHARED, fd, 0);
    if (map){
      int ofs;

      if (re != NULL) {
	regex_t preg;
	regmatch_t pmatch[1];
	int errcode;
	char *m;

	errcode=regcomp(&preg, re, REG_EXTENDED);
	if (errcode != 0) {
	  char errbuf[100];
	  regerror(errcode,&preg,errbuf,sizeof(errbuf));
	  write(2,"could not compile regular expression: ",38);
	  die(errbuf,0);
	}
	pmatch[0].rm_so=-1; pmatch[0].rm_eo=-1; m=map-1;
	while ((errcode=regexec(&preg, m+1, 1, pmatch, 0)) == 0) {
	  if (pmatch[0].rm_so != -1) {
	    m=printquote(map, m+pmatch[0].rm_so, len);
	    write(1,"%\n",2);
	  }
	}
	if (errcode != REG_NOMATCH) {
	  die("could not compile regular expression",0);
#if 0
	  errbuf=regerr(&preg, errcode);
	  fprintf(stderr,
		  "%s: couldn't match regular expression `%s': %s\n",
		  progname, re,errbuf);
	  free(errbuf);
#endif
	}
	regfree(&preg);
      } else {
	ofs = rand() % len;
	printquote(map, map + ofs, len);
      }
      retcode = 0;
    } else {
      die("could not mmap",filename);
#if 0
      fprintf(stderr, "%s: could not mmap `%s'!\n", progname, filename);
#endif
    }
    munmap(map, len);
    close(fd);
  } else {
    die("could not open",filename);
#if 0
    fprintf(stderr, "%s: could not open `%s'!\n", progname, filename);
#endif
  }

  return retcode;
}

int main(int argc, char *argv[])
{
  int fd;
  int retcode;
  int c;
  char *re;

  progname=argv[0]; re=NULL;
  while ((c = getopt_long(argc, argv, "h?m:V",
			  long_options, (int *) 0)) != -1) {
    switch (c) {
      case 0  : break;
      case 'h':
      case '?': carp("Usage: fortune [-hV] [-m regex] [filename...]",0);
                return 0;
      case 'V': carp("fortune " VERSION "\n",0);
		return 0;
      case 'm': re=optarg;
      default : break;
    }
  }

#if 0
  setlocale(LC_CTYPE, "");
#endif

  if ((fd = open("/dev/urandom", O_NDELAY)) >= 0) {
    int foo;
    if (read(fd, &foo, sizeof(int)) > 0)
      srand(foo);
    else
      srand(time(0));
  } else
    srand(time(0));

  /* Were some filenames given as arguments? */
  if (argc > optind) {
    char *filename;

    if (re == NULL)
      filename = argv[(rand() % (argc - optind)) + optind];
    else
      filename = argv[optind];

    retcode = printormatchfortune(filename, re);
  } else {
    /* No? Then search the fortune directory. */
    DIR *d;

    d=opendir(FORTUNEDIR);
    if (d == NULL) {
      carp("could not open directory",FORTUNEDIR);
      retcode=1;
    } else {
      struct dirent *e;
      char *files=NULL;
      int fileno=0;

      retcode=0;

      chdir(FORTUNEDIR);
      while ((e=readdir(d)) != NULL) {
	struct stat sbuf;

	if (lstat(e->d_name, &sbuf) < 0) {
	  carp("could not stat",e->d_name);
	} else {
	  if (S_ISREG(sbuf.st_mode)) {
	    if (re != NULL) {
	      write(2,"%(",2);
	      write(2,e->d_name,strlen(e->d_name));
	      write(2,")\n",2);
	      retcode |= printormatchfortune(e->d_name, re);
	    } else {
	      files=(char *)realloc(files,(fileno+1)*NAME_MAX);
	      strcpy(&files[fileno*NAME_MAX],e->d_name);
	      fileno++;
	    }
	  }
	}
      }

      closedir(d);
      if (re == NULL) {
	printormatchfortune(&files[(rand() % fileno)*NAME_MAX], re);
	free(files);
      }
    }
  }
  return retcode;
}
