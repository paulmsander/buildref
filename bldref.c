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
#include <btree.h>
#include <readln.h>
#include "bldref.h"

extern int optind;
extern int opterr;
extern int optopt;
extern char *optarg;

extern int strcmp();

/* Build reference structure */
struct ref {
	char		*path;		/* Path of build, on heap, indexed */
	int			nest;		/* Test nested references if set */
	int			seen;		/* Path seen while accumulating */
	char		listed;		/* Path seen while listing */
};
typedef struct ref REF, *REFP;

/* Build reference list */
struct reflist {
	REFP			ref;	/* Build reference */
	struct reflist	*next;	/* Next reference in list */
};
typedef struct reflist REFLIST, *REFLISTP;

char		msgFile[] = "bldref.msg";	/* Name of XPG3 message file */
char		Makefile[] = "Makefile";	/* Name of Makefile */
char		bldref[] = ".bldref";		/* Name of .bldref file */
char		nbldref[] = ".bldref.new";	/* Name of new .bldref file */
char		obldref[] = ".bldref.old";	/* Name of old .bldref file */

int			status = 0;				/* Exit status */
char		*progname;				/* Program name */
int			append = 0;				/* Append to .bldref file, not prepend */
char		*dir = NULL;			/* Directory containing .bldref file */
int			list = 0;				/* List contents of .bldref file */
int			rmref = 0;				/* Remove directories from .bldref file */
static int	clear = 0;				/* Clear ref list */
int			debug = 0;				/* Debugging */
int			All = 0;				/* Transitive closure on args */
BTREE		refidx;					/* Reference index */
REFLISTP	refhead = NULL;			/* Head of reference list */
REFLISTP	reftail = NULL;			/* Tail of reference list */
int			filecnt = 0;			/* Counts .bldref files, finds dups */


/*****************************************************************
 *
 * strdup -- Replicate a string on the heap
 *
 *****************************************************************/

#ifdef __STDC__
char *strdup(const char *p)
#else
char *strdup(p)
char	*p;
#endif
{
	char	*r;

	r = (char*) malloc(strlen(p)+1);
	if ( r != NULL ) strcpy(r,p);
	return r;
}

/*****************************************************************
 *
 * getMsg -- Get message from message catalog and return it on the heap.
 *
 * msgno = message number from messages.h
 *
 *****************************************************************/

char errMsg[] = "Cannot open message file %s,\n\
check NLSPATH environment variable [catopen(3c)]\n\
Message #%d";

char defaultMsg[] = "Message #%d";

#ifdef __STDC__
char *getMsg(int msgno)
#else
char *getMsg(msgno)
int	msgno;
#endif
{
	nl_catd		ef;		/* Message catalog */
	char		*msg;	/* Message from catalog */
	static char	b[sizeof(errMsg) + sizeof(msgFile) + sizeof(int) * 3];

	ef = catopen(msgFile,0);
	if ( ef == (nl_catd) -1 )
	{
		/* Default message if catalog not available */
		(void) sprintf(b,errMsg,msgFile,msgno);
		msg = strdup(b);
	}
	else
	{
		/* Build message from catalog */
		(void) sprintf(b,defaultMsg,msgno);
		msg = strdup(catgets(ef,1,msgno,b));
		catclose(ef);
	}
	if ( msg == NULL )
	{
		fprintf(stderr,"Memory allocation failure in error reporting:\n");
		perror(NULL);
		fprintf(stderr,"Message #%d\n",msgno);
		exit(2);
	}
	return msg;
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
	char		*msg;	/* Message from catalog */
	va_list		ap;	/* Scans arguments */

	fprintf(stderr,"%s: ",progname);
	msg = getMsg(msgno);
#ifdef __STDC__
	va_start(ap,msgno);
#else
	va_start(ap);
#endif
	vfprintf(stderr,msg,ap);
	va_end(ap);
	free(msg);

	if ( syserr )
	{
		/* Show system error */
		fprintf(stderr,":\n");
		perror(NULL);
	}
	fprintf(stderr,"\n");

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
 * freeRef -- Free build reference structure
 *
 *****************************************************************/

/* ARGSUSED */
#ifdef __STDC__
void freeRef(REFP ref, void *info)
#else
void freeRef(ref,info)
REFP	ref;
void	*info;
#endif
{
	if ( ref->path ) free(ref->path);
	free(ref);
}

/*****************************************************************
 *
 * addRef -- Add reference to index and list
 *
 * path = path to root of build
 * fpath = path to .bldref file containing path
 * lno = line number in fpath
 * nest = test recursively for indirect references if true
 * cnt = counter of .bldref files for detecting duplicates
 * rm = remove reference from .bldref file
 *
 *****************************************************************/

#ifdef __STDC__
int addRef(char *path,char *fpath, int lno, int nest, int cnt, int rm)
#else
int addRef(path,fpath,lno,nest,cnt,rm)
char	*path;
char	*fpath;
int		lno;
int		nest;
int		cnt;
int		rm;
#endif
{
	int			status;		/* Return value */
	int			st;			/* Return value */
	size_t		len;		/* Length of path */
	REFP		ref;		/* Build reference structure */
	REFLISTP	refnode;	/* Build reference list node */
	int			res;		/* B-tree insert result */
	char		*fnd;		/* B-tree search result */

	if ( debug )
	{
		fprintf(stderr,"addRef:\n");
		fprintf(stderr,"path = %s\n",path);
		fprintf(stderr,"fpath = %s\n",fpath);
		fprintf(stderr,"lno = %d\n",lno);
		fprintf(stderr,"nest = %d\n",nest);
		fprintf(stderr,"cnt = %d\n",cnt);
		fprintf(stderr,"\n");
	}

	/* Test length of bldref to be sure that .bldref file fullpath is valid */
	status = 0;
	len = strlen(path);
	if ( len + sizeof(bldref) + 1 > MAXPATHLEN )
	{
		pmsg(0,0,RefTooLong,len,MAXPATHLEN-sizeof(bldref)-1,path,fpath,lno);
		status = 1;
	}
	else
	{
		/* Build reference structure */
		ref = (REFP) malloc(sizeof(REF));
		if ( ref == NULL )
		{
			pmsg(2,1,NoRefStruct,fpath,lno);
		}
		ref->seen = cnt;
		if ( rm )
		{
			ref->nest = 0;
			ref->listed = 1;
		}
		else
		{
			ref->nest = nest;
			ref->listed = 0;
		}
		ref->path = strdup(path);
		if ( ref->path == NULL )
		{
			pmsg(2,1,NoDupPath,path,fpath,lno);
		}

		/* Add reference to index */
		res = bt_insert(refidx,ref->path,ref);
		if ( res == 0 )
		{
			/* Failed to add */
			pmsg(2,1,BtInsert,path,fpath,lno);
		}
		else if ( res < 0 )
		{
			/* Already there -- use existing ref and free the new one */
			freeRef(ref,NULL);
			fnd = (char*) bt_search(refidx,path,(void**)&ref);
			if ( fnd == NULL )
			{
				pmsg(2,1,BtDup,path,fpath,lno);
			}

			/* Warn if duplicate */
			if ( ref->seen == cnt )
			{
				pmsg(0,0,DupPath,path,fpath,lno);
				status = 1;
			}

			/* No need to check nested references again */
			ref->nest = 0;
		}
		else
		{
			if ( debug )
			{
				fprintf(stderr,"Successful add\n");
			}
		}

		/* Insert into reference list */
		refnode = (REFLISTP) malloc(sizeof(REFLIST));
		if ( ref == NULL )
		{
			pmsg(2,1,NoRefNode,fpath,lno);
		}
		refnode->ref = ref;
		refnode->next = NULL;
		if ( reftail != NULL ) reftail->next = refnode;
		reftail = refnode;
		if ( refhead == NULL ) refhead = refnode;
	}
	if ( debug )
	{
		fprintf(stderr,"Leaving addRef\n\n");
	}
	return status;
}

/*****************************************************************
 *
 * Read an open .bldref file, build reference list and index.
 *
 * f = buffered file handle of open .bldref file
 * cnt = .bldref file counter for detecting duplicate references
 * fpath = path of open .bldref file
 * nest = test recursively for indirect references if true
 *
 *****************************************************************/

#ifdef __STDC__
int readBldref(FILE *f, int cnt, char *fpath, int nest)
#else
int readBldref(f,cnt,fpath,nest)
FILE	*f;
int		cnt;
char	*fpath;
int		nest;
#endif
{
	char		*line;		/* Path from .bldref file */
	int			lno;		/* .bldref file line counter */
	int			status;		/* Return value */
	int			st;			/* Return value */


	/* Initialize read loop */
	status = 0;
	line = freadln(f);
	lno = 1;
	while ( line != NULL )
	{
		st = addRef(line,fpath,lno,nest,cnt,0);
		if ( status == 0 ) status = st;

		/* Next one */
		line = freadln(f);
		lno++;
	}
	return status;
}

/*****************************************************************
 *
 * readFile -- Open a .bldref file and read its contents.
 *
 * path = path to .bldref file
 * filecnt = .bldref file counter for detecting duplicate refs
 * nest = test recursively for indirect references if true
 *
 *****************************************************************/

#ifdef __STDC__
int readFile(char *path, int filecnt, int nest)
#else
int readFile(path,filecnt,nest)
char	*path;
int		filecnt;
int		nest;
#endif
{
	int		st;
	FILE	*f;

	st = 0;
	if ( debug )
	{
		fprintf(stderr,"readFile:\n");
		fprintf(stderr,"path = %s\n",path);
		fprintf(stderr,"clear = %d\n",clear);
		fprintf(stderr,"filecnt = %d\n",filecnt);
		fprintf(stderr,"nest = %d\n",nest);
		fprintf(stderr,"\n");
	}
	f = fopen(path,"r");
	if ( f == NULL )
	{
#if 0
		pmsg(0,1,CantOpenBldref,path);
#endif
	}
	else
	{
		st = readBldref(f,filecnt,path,nest);
		fclose(f);
	}
	if ( debug )
	{
		fprintf(stderr,"Leaving readFile\n\n");
	}
	return st;
}

/*****************************************************************
 *
 * dirRefs -- Read build references listed in a directory's .bldref
 *            file.
 *
 * dir = path to directory containing .bldref file
 * filecnt = .bldref file counter for detecting duplicate refs
 *
 *****************************************************************/

#ifdef __STDC__
int dirRefs(char *dir, int filecnt)
#else
int dirRefs(dir,filecnt)
char	*dir;
int		filecnt;
#endif
{
	int		st;					/* Return status */
	char	path[MAXPATHLEN];	/* Path of .bldref file */

	(void) sprintf(path,"%s/%s",dir,bldref);
	st = readFile(path,filecnt,1);
	return st;
}

/*****************************************************************
 *
 * argRefs -- Add command line references to index and list
 *
 * argc = number of arguments
 * argv = list of paths to directories containing .bldref files
 *
 *****************************************************************/

#ifdef __STDC__
int argRefs(int argc, char **argv, int nest)
#else
int argRefs(argc,argv,nest)
int		argc;
char	**argv;
int		nest;
#endif
{
	int		st;					/* Return status */
	int		st2;				/* Return status */
	int		i;					/* Loop counter */
	char	path[MAXPATHLEN];	/* Path of .bldref file */
	int		l;					/* Length of argument */
	char	*cmdline;			/* "Command Line" */

	if ( debug )
	{
		fprintf(stderr,"Entering argRefs\n\n");
	}

	/* Get "Command Line" from message file for diagnostics */
	cmdline = getMsg(CommandLine);
	st = 0;
	for ( i = 0; i < argc; i++ )
	{
		/* Verify that path is short enough */
		l = strlen(argv[i]);
		if ( l + sizeof(bldref) + 1 > sizeof(path) )
		{
			st=2;
			pmsg(0,0,ArgTooLong,l,sizeof(path)-sizeof(bldref)-1,argv[i]);
		}
		else
		{
			/*
			 * Add to reference list; filecnt = 0 for cmdline, recursive
			 * references, no line count
			 */
			st2 = addRef(argv[i],cmdline, 0, nest, 0, rmref);
			if ( st == 0 ) st = st2;
		}
	}

	/* Clean-up */
	free(cmdline);
	if ( debug )
	{
		fprintf(stderr,"Leaving argRefs\n\n");
	}
	return st;
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
	int			st;						/* Return status */
	char		badopt[2];				/* Bad option, converted to string */
	int			i;						/* Loop counter */
	struct stat	stbuf;					/* For testing existence of files */
	FILE		*f;						/* For Make, reading .bldref */
	char		*line;					/* Line of input */
	char		*line2;					/* More input */
	char		topdir[MAXPATHLEN];		/* Output of "make .top" */
	char		path[MAXPATHLEN];		/* Path to .bldref file */
	char		path2[MAXPATHLEN];		/* Path to new/old .bldref file */
	BT_SETUP	bsetup;					/* For reference index */
	REFLISTP	ref;					/* Traverses reference list */
	REFLISTP	next;					/* Traverses reference list */

	/* Initialize badopt[] */
	badopt[0] = 0;
	badopt[1] = 0;

	/* Parse command line */
	progname = argv[0];
	opterr = 0;
	while ( ( c = getopt(argc,argv,"Aacd:hlrX") ) != EOF )
	{
		switch (c)
		{

	case 'a':
			append = 1;
			break;

	case 'A':
			All = 1;
			break;

	case 'c':
			clear = 1;
			break;

	case 'd':
			dir = optarg;
			break;

	case 'h':
			pmsg(0,0,Usage);
			exit(0);

	case 'l':
			list = 1;
			break;

	case 'r':
			rmref = 1;
			break;

	case 'X':
			debug = 1;
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

	if ( debug )
	{
		fprintf(stderr,"append = %d\n",append);
		fprintf(stderr,"clear = %d\n",clear);
		fprintf(stderr,"dir = %s\n",( dir == NULL ? "NULL" : dir ));
		fprintf(stderr,"list = %d\n",list);
		fprintf(stderr,"rmref = %d\n",rmref);
		fprintf(stderr,"arguments:\n");
		for ( i = 0; i < argc; i++ )
		{
			fprintf(stderr,"  %s\n",argv[i]);
		}
	}

	/* Error if no-op */
	if ( ( list == 0 ) && ( clear == 0 ) && ( argc <= 0 ) )
	{
		pmsg(2,0,NoOp);
	}

	/* Determine directory containing the .bldref file */
	if ( dir == NULL )
	{
		/* Try "make .top" */
		if ( stat(Makefile,&stbuf) >= 0 )
		{
			/*
			 * Makefile is present, run Make.  Though using popen is sleazy,
			 * it's easy.
			 */
			if ( debug )
			{
				fprintf(stderr,"Invoking Make\n");
			}
			f = (FILE*) popen("make .top 2>&1","r");
			if ( f == NULL )
			{
				/* Can't run popen, bad news */
				pmsg(2,1,CantMake);
			}
			else
			{
				/* Make is running, read its output */
				line = freadln(f);
				if ( debug )
				{
					fprintf(stderr,"Make running:\n");
					if ( line != NULL ) fprintf(stderr,"%s\n",line);
				}
				line2 = line;
				while ( line2 != NULL )
				{
					line2 = freadln(f);
					if ( debug && ( line2 != NULL ) )
					{
						fprintf(stderr,"%s\n",line2);
					}
				}
				i = pclose(f);
				if ( debug )
				{
					fprintf(stderr,"Exit status of Make:  %d\n",i);
				}
				if ( i != 0 )
				{
					/* Failed to do a "make .top", assume "." */
					dir = ".";
				}
				else
				{
					/* "make .top" succeeded, use its output */
					if ( strlen(line) > sizeof(topdir) - sizeof(nbldref) - 2 )
					{
						pmsg(2,0,TopTooLong,sizeof(topdir)-sizeof(nbldref)-2);
					}
					strcpy(topdir,line);
					dir = topdir;
				}
			}
		}
		else
		{
			/* Can't stat the Makefile */
			if ( errno == ENOENT )
			{
				/* No Makefile, assume "." */
				if ( debug )
				{
					fprintf(stderr,"No Makefile present\n");
				}
				dir = ".";
			}
			else
			{
				/* Something seriously wrong */
				pmsg(2,1,NoMakefile);
			}
		}
	}
	if ( debug )
	{
		fprintf(stderr,"dir = %s\n",dir);
	}

	/* Compute path of user's .bldref file */
	(void) sprintf(path,"%s/%s",dir,bldref);
	if ( debug )
	{
		fprintf(stderr,"Path of .bldref = %s\n",path);
	}

	/* Create reference index */
#ifdef __STDC__
	bsetup = bt_setup(7,(int(*)(void*,void*))strcmp,NULL);
#else
	bsetup = bt_setup(7,strcmp,NULL);
#endif
	if ( bsetup == NULL )
	{
		pmsg(2,1,SetupIndex);
	}
	refidx = bt_new(bsetup);
	if ( refidx == NULL )
	{
		pmsg(2,1,RefIndex);
	}
	bt_freeSetup(bsetup);

	/* Read the .bldref file and command line arguments */
	filecnt = 1;
	if ( append )
	{
		if ( ! clear )
		{
			st = readFile(path,filecnt,0);
			if ( status == 0 ) status = st;
			filecnt++;
		}
		st = argRefs(argc,argv,All);
		if ( status == 0 ) status = st;
	}
	else
	{
		st = argRefs(argc,argv,All);
		if ( status == 0 ) status = st;
		if ( ! clear )
		{
			st = readFile(path,filecnt,0);
			if ( status == 0 ) status = st;
			filecnt++;
		}
	}

	/*
	 * Now that all of the direct references are known, look for indirect
	 * ones by scanning the existing list of references and scanning the
	 * .bldref file for each reference that has its "nest" flag set.
	 * The "nest" flag is cleared when a .bldref file is scanned to
	 * prevent infinite recursion.
	 */

	if ( debug )
	{
		fprintf(stderr,"Checking indirect references\n\n");
	}

	/* Begin at the head of the reference list */
	ref = refhead;
	while ( ref != NULL )
	{
		if ( debug )
		{
			fprintf(stderr,"Checking %s\n",ref->ref->path);
		}

		/* Test recursively? */
		if ( ref->ref->nest )
		{
			st = dirRefs(ref->ref->path,filecnt);
			if ( status == 0 ) status = st;
			filecnt++;
			ref->ref->nest = 0;
		}
		else
		{
			if ( debug )
			{
				fprintf(stderr,"Nesting disabled\n\n");
			}
		}
		ref = ref->next;
	}
	if ( debug )
	{
		fprintf(stderr,"Done checking indirect references\n\n");
	}

	if ( list && ! clear && ( argc <= 0 ) )
	{
		ref = refhead;
		while ( ref != NULL )
		{
			if ( ! ref->ref->listed )
			{
				puts(ref->ref->path);
			}
			ref->ref->listed = 1;
			ref = ref->next;
		}
	}
	else
	{
		/* Link .bldref file to .bldref.old file */
		(void) sprintf(path2,"%s/%s",dir,obldref);
		if ( stat(path2,&stbuf) >= 0 )
		{
			/* File exists, remove it */
			if ( unlink(path2) < 0 )
			{
				pmsg(2,1,CantUnlink,path2);
			}
		}
		else
		{
			if ( errno != ENOENT )
			{
				pmsg(2,1,CantStat,path2);
			}
		}
		if ( link(path,path2) < 0 )
		{
			if ( errno != ENOENT )
			{
				pmsg(2,1,CantLink,path,path2);
			}
		}
		
		/* Write reference list, avoiding duplicates */
		(void) sprintf(path2,"%s/%s",dir,nbldref);
		f = fopen(path2,"w");
		if ( f == NULL )
		{
			pmsg(2,1,CantOpen,path2);
		}
		ref = refhead;
		while ( ref != NULL )
		{
			if ( ! ref->ref->listed )
			{
				if ( list ) puts(ref->ref->path);
				if ( fputs(ref->ref->path,f) == EOF )
				{
					pmsg(2,1,CantWrite,path2);
				}
				if ( fputs("\n",f) == EOF )
				{
					pmsg(2,1,CantWrite,path2);
				}
			}
			ref->ref->listed = 1;
			ref = ref->next;
		}
		fclose(f);

		/* Shuffle new .bldref to replace the old one */
		if ( rename(path2,path) < 0 )
		{
			pmsg(2,1,CantRename,path,path2);
		}
	}

	/* Destroy the reference index */
#ifdef __STDC__
	bt_destroy(refidx,NULL,(void(*)(void*,void*))freeRef,NULL);
#else
	bt_destroy(refidx,NULL,freeRef,NULL);
#endif

	/* Destroy the reference list */
	ref = refhead;
	while ( ref != NULL )
	{
		next = ref->next;
		free(ref);
		ref = next;
	}

	xfreeln();
	exit(status);
}
