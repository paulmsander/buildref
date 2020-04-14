#include <sys/param.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <nl_types.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlist.h>
#include <readln.h>
#include "paths.h"

extern int optind;
extern int opterr;
extern int optopt;
extern char *optarg;

#ifdef __STDC__
extern int strcmp(const char*, const char*);
#define STRCMPTYPE int(*)(void*, void*)
#else
extern int strcmp();
#define STRCMPTYPE int(*)()
#endif

char	*msgFile = "paths.msg";	/* Name of XPG3 message file */
char	*searchLoc = ".bldstate/search";

char	*defprefix = "";		/* Prefix edit of output */
char	*prefix;				/* Prefix edit of output */
char	*defsuffix = "";		/* Suffix edit of output */
char	*suffix;				/* Suffix edit of output */
char	*defseparator = "\n";	/* Separator of output */
char	*separator;				/* Separator of output */
int		all = 0;				/* Show all paths */
int		debug = 0;				/* Debugging */
int		list = 0;				/* List classes */
int		status = 0;				/* Exit status */
char	*progname;				/* Program name */

/*****************************************************************
 *
 * strdup -- Duplicate a string on the heap
 *
 *****************************************************************/

#ifdef __STDC__
char *strdup(const char *p)
#else
char *strdup(p)
char *p;
#endif
{
	char	*r;

	r = (char*) malloc(strlen(p)+1);
	if ( r ) strcpy(r,p);
	return r;
}

/*****************************************************************
 *
 * pmsg -- Display error message, maybe exit.
 *
 * status = exit status if non-zero
 * syserr = display errno message if non-zero
 * msgno  = message from XPG3 message file
 *
 *****************************************************************/

#ifdef __STDC__
void pmsg(int status, int syserr, int msgno, ...)
#else
void pmsg(status, syserr, msgno, va_alist)
int	status;
int	syserr;
int	msgno;
va_dcl
#endif
{
	nl_catd		ef;		/* Message catalog */
	char		*msg;	/* Message from catalog */
	char		b[80];	/* Default message if message unavailable */
	va_list		ap;	/* Scans arguments */

	fprintf(stderr,"%s: ",progname);
	ef = catopen(msgFile,0);
	if ( ef == (nl_catd) -1 )
	{
		/* Default message if catalog not available */
		fprintf(stderr,"Cannot open message file %s,\n",msgFile);
		fprintf(stderr,"check NLSPATH environment variable [catopen(3c)]\n");
		fprintf(stderr,"Message #%d",msgno);
	}
	else
	{
		/* Build message from catalog */
		(void) sprintf(b,"Message #%d",msgno);
		msg = catgets(ef,1,msgno,b);
#ifdef __STDC__
		va_start(ap,msgno);
#else
		va_start(ap);
#endif
		vfprintf(stderr,msg,ap);
		va_end(ap);
		catclose(ef);
	}

	if ( syserr )
	{
		/* Show system error */
		fprintf(stderr,":\n");
		perror(NULL);
	}
	else
	{
		fprintf(stderr,"\n");
	}

	/* Exit if needed */
	if ( status ) exit(status);

	return;
}

/*****************************************************************
 *
 * free2 -- Frees key during ChipSet destroy routines
 *
 *****************************************************************/

/* ARGSUSED */
#ifdef __STDC__
void free2(void *item, void *info)
#else
void free2(item,info)
void	*item;
void	*info;
#endif
{
	free(item);
}

/*****************************************************************
 *
 * main -- Here is where the action begins.
 *
 *****************************************************************/

#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc,argv)
int		argc;
char	**argv;
#endif
{
	int			c;						/* Command line option */
	char		badopt[2];				/* Bad option, converted to string */
	char		*bldrefp = NULL;		/* bldref file path */
	char		*classp = NULL;			/* search class */
	FILE		*bldref;				/* bldref file */
	FILE		*search;				/* bldref file */
	char		*line;					/* Line of text */
	DLL_SETUP	dls;					/* Temp setup structure */
	DL_LIST		reflist;				/* List of directories to search */
	DL_LIST		bldlist;				/* List of build refs */
	char		*ref;					/* Temporary */
	char		searchFile[MAXPATHLEN];	/* Location of search file */
	char		*bpath;					/* Path to build */
	char		spath[MAXPATHLEN];		/* Path to search file */
	char		path[MAXPATHLEN];		/* Path to search */
	char		*class;					/* Type of path */
	int			bopts = 0;				/* Set if -b is given */
	int			i;						/* Loop counter */
	int			fnd;					/* Found file */
	int			fnd1;					/* Found any file */
	int			r;						/* Result of stat */
	struct stat	st;						/* Status buffer */
	char		*sep;					/* Separator */
	int			bad = 0;				/* Incompatible arguments */

	prefix = defprefix;
	suffix = defsuffix;
	separator = defseparator;

	badopt[0] = 0;
	badopt[1] = 0;

	/* Save program name for diagnostics */
	progname = argv[0];

	/* Create bldref list in case -b options are given */
	dls = dll_setup((STRCMPTYPE)strcmp,NULL);
	if ( dls == NULL ) pmsg(2,1,NoSetup);
	bldlist = dll_new(dls);
	if ( bldlist == NULL ) pmsg(2,1,NoBldRefList);

	/* Parse command line */
	opterr = 0;
	while ( ( c = getopt(argc,argv,"ab:dhlp:s:S:") ) != EOF )
	{
		switch (c)
		{

	case 'a':
			/* Show all paths, not just the first */
			all = 1;
			break;

	case 'b':
			/* Add this to the bldref list */
			bopts = 1;
			if ( strlen(optarg) + strlen(searchLoc) + 1 >= (size_t) MAXPATHLEN )
			{
				pmsg(0,0,SearchTooLong,MAXPATHLEN-strlen(optarg)-1);
				status = 2;
			}
			else
			{
				line = strdup(optarg);
				if ( line == (char*) NULL )
				{
					pmsg(2,1,NoMem,optarg);
				}
				if ( debug ) fprintf(stderr,"Have -b %s\n",optarg);
				if ( dll_pushr(bldlist,line,NULL) == NULL )
				{
					free(line);
					pmsg(2,1,BadBldRefList,optarg);
				}
			}
			break;

	case 'd':
			/* Debugging */
			debug = 1;
			break;

	case 'h':
			/* Help */
			pmsg(0,0,Usage);
			exit(0);

	case 'l':
			/* List classes */
			list = 1;
			break;

	case 'p':
			/* Prefix edit */
			prefix = optarg;
			break;

	case 's':
			/* Suffix edit */
			suffix = optarg;
			break;

	case 'S':
			/* Separator */
			separator = optarg;
			break;

	default:
			/* Error */
			badopt[0] = optopt;
			pmsg(2,0,BadOpt,badopt);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	/* Validate combinations of arguments */
	if ( all && list )
	{
		pmsg(0,0,LandA);
		bad = 1;
	}

	if ( list && ( prefix != defprefix ) )
	{
		pmsg(0,0,LandP);
		bad = 1;
	}

	if ( list && ( suffix != defsuffix ) )
	{
		pmsg(0,0,LandS);
		bad = 1;
	}

	if ( list && ( separator != defseparator ) )
	{
		pmsg(0,0,LandCapS);
		bad = 1;
	}

	/* Read name of .bldref file from command line if no -b options */
	if ( ! bopts )
	{
		if ( argc <= 0 ) pmsg(2,0,NoBldref);
		bldrefp = argv[0];
		++argv;
		--argc;

		/* Read bldref and search files, build search path in list */
		if ( debug ) fprintf(stderr,"bldrefp = %s\n",bldrefp);
		bldref = fopen(bldrefp,"r");
		if ( bldref == (FILE*) NULL ) pmsg(2,1,BadBldref,bldrefp);
		line = freadln(bldref);
		while ( line != NULL )
		{
			if ( debug ) fprintf(stderr,"Reference to %s\n",line);

			/* Validate path */
			if ( strlen(line) + strlen(searchLoc) + 1 >= (size_t) MAXPATHLEN )
			{
				pmsg(0,0,SearchTooLong,MAXPATHLEN-strlen(line)-1);
				status = 2;
			}
			else
			{
				/* Open search file */
				(void) strcpy(spath,line);
				line = strdup(spath);
				if ( line == NULL )
				{
					pmsg(2,1,NoMem,spath);
				}
				if ( dll_pushr(bldlist,line,NULL) == NULL )
				{
					free(line);
					pmsg(2,1,BadBldRefList,spath);
				}
			}
			line = freadln(bldref);
		}
		fclose(bldref);
	}

	if ( list )
	{
		if ( argc > 0 )
		{
			pmsg(0,0,LandClass);
			bad = 1;
		}
	}
	else
	{
		if ( argc <= 0 ) pmsg(2,0,NoClass);
		classp = argv[0];
		++argv;
		--argc;
	}

	if ( debug )
	{
		fprintf(stderr,"prefix = %s\n",prefix);
		fprintf(stderr,"suffix = %s\n",suffix);
		fprintf(stderr,"separator = %s\n",separator);
		fprintf(stderr,"all = %d\n",all);
		fprintf(stderr,"list = %d\n",list);
		if ( ! list ) fprintf(stderr,"classp = %s\n",classp);
		fprintf(stderr,"bad = %d\n",bad);
	}

	if ( bad )
	{
		pmsg(2,0,Usage);
	}

	/* Create search list */
	reflist = dll_new(dls);
	if ( reflist == NULL ) pmsg(2,1,NoBldRefList);

	/* Read search files, build search path in list */
	bpath = (char*) dll_first(bldlist,NULL);
	while ( bpath != NULL )
	{
		if ( debug ) fprintf(stderr,"Checking reference to %s:\n",bpath);
		(void) sprintf(spath,"%s/%s",bpath,searchLoc);
		search = fopen(spath,"r");
		if ( search == (FILE*) NULL ) pmsg(2,1,BadSearch,spath);
		line = freadln(search);
		while ( line != NULL )
		{
			if ( debug ) fprintf(stderr,"  %s\n",line);

			/* Validate syntax */
			class = strtok(line," \t");
			if ( class != NULL )
			{
				ref = strtok(NULL," \t");
				if ( ref == NULL )
				{
					pmsg(0,0,BadPath,class,spath);
					status = 2;
				}
				else if ( list )
				{
					/* Gather all classes into a list for later display */
					line = strdup(class);
					if ( line == NULL )
					{
						pmsg(2,1,NoMem,ref);
					}
					if ( debug ) fprintf(stderr,"Inserting %s\n",line);
					i = dll_insert(reflist,line,NULL);
					if ( debug ) fprintf(stderr,"Result is %d\n",i);
					if ( i == 0 )
					{
						free(line);
						pmsg(2,1,BadBldRefList,class);
					}
					else if ( i == -1 )
					{
						free(line);
					}
				}
				else if ( ref[0] == '/' )
				{
					/* Validate path */
					if ( strlen(ref)+1 >= (size_t) MAXPATHLEN )
					{
						pmsg(0,0,Path1TooLong,ref);
						status = 2;
					}
					/* Add to search list */
					else if ( strcmp(classp,class) == 0 )
					{
						if ( debug ) fprintf(stderr,"  (match)\n");
						line = strdup(ref);
						if ( line == NULL )
						{
							pmsg(2,1,NoMem,ref);
						}
						if ( debug ) fprintf(stderr,"Inserting %s\n",line);
						if ( dll_pushr(reflist,line,NULL) == NULL )
						{
							free(line);
							pmsg(2,1,BadBldRefList,ref);
						}
					}
				}
				else
				{
					/* Validate path */
					if ( strlen(bpath)+strlen(ref)+1 >= (size_t) MAXPATHLEN )
					{
						pmsg(0,0,PathTooLong,bpath,ref,bpath);
						status = 2;
					}
					/* Add to search list */
					else if ( strcmp(classp,class) == 0 )
					{
						if ( debug ) fprintf(stderr,"  (match)\n");
						(void) sprintf(path,"%s/%s",bpath,ref);
						line = strdup(path);
						if ( line == NULL )
						{
							pmsg(2,1,NoMem,path);
						}
						if ( debug ) fprintf(stderr,"Inserting %s\n",line);
						if ( dll_pushr(reflist,line,NULL) == NULL )
						{
							free(line);
							pmsg(2,1,BadBldRefList,path);
						}
					}
				}
			}
			line = freadln(search);
		}
		fclose(search);
		bpath = (char*) dll_next(bldlist,NULL);
	}

	if ( status == 0 )
	{
		if ( debug )
		{
			fprintf(stderr,"argc is %d\n",argc);
		}

		/* If -l was given then dump list of classes */
		if ( list )
		{
			line = dll_first(reflist,NULL);
			while ( line != NULL )
			{
				printf("%s\n",line);
				line = dll_next(reflist,NULL);
			}
		}

		/* No files, dump list */
		else if ( argc <= 0 )
		{
			sep = "";
			line = (char*) dll_first(reflist,NULL);
			while ( line != NULL )
			{
				printf("%s%s%s%s",sep,prefix,line,suffix);
				line = (char*) dll_next(reflist,NULL);
				sep = separator;
			}
			printf("\n");
		}
		/* Files given, find paths */
		else
		{
			sep = "";
			fnd1 = 0;
			for ( i = 0; i < argc; i++ )
			{
				fnd = 0;
				line = (char*) dll_first(reflist,NULL);
				while ( line != NULL )
				{
					if ( strlen(argv[i])+strlen(line)+1 > (size_t) MAXPATHLEN )
					{
						pmsg(0,0,BadFilePath,argv[i],line,argv[i]);
						status = 2;
					}
					else
					{
						(void) sprintf(path,"%s/%s",line,argv[i]);
						if ( debug ) fprintf(stderr,"Testing %s\n",path);
						r = stat(path,&st);
						if ( ( r < 0 ) && ( errno != ENOENT ) )
						{
							pmsg(0,1,BadFile,path);
							status = 2;
						}
						else if ( r >= 0 )
						{
							fnd = 1;
							fnd1 = 1;
							printf("%s%s%s%s",sep,prefix,path,suffix);
							sep = separator;
							if ( ! all ) break;
						}
					}
					line = (char*) dll_next(reflist,NULL);
				}
				if ( ! fnd )
				{
					pmsg(0,0,NoFile,argv[i]);
					status = 1;
				}
			}
			if ( fnd1 )
			{
				printf("\n");
			}
		}
	}

	dll_destroy(reflist,free2,NULL,NULL);
	dll_destroy(bldlist,free2,NULL,NULL);
	xfreeln();
	exit(status);
}

