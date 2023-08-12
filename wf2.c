/*
 * Program   : wf (wipefile)
 *
 * Purpose   : Fills a fill with null characters, and then removes
 *             the file when done. This performs a "security erase".
 *
 * Arguments : <file-1> <file-2> ... <file-n>
 *
 * Usage     : wf *.jpg fred.bat
 *
 * History   : 15/04/97 Dean      First revision.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <ctype.h>
#include <dos.h>
#include <dir.h>
#include <sys\stat.h>

#define TRUE		1
#define FALSE		0

typedef int BOOLEAN;

typedef struct file_tag
{
   char fname[13];
   struct file_tag *next;
} FILELIST;


FILELIST
*BuildList(FILELIST *, BOOLEAN, char *);

BOOLEAN
directory(char);

void
Help(void);

BOOLEAN
FileSpecMatches(char *, char *);

BOOLEAN
PatternMatches(char *, char *);

BOOLEAN
SearchDir(char *);

BOOLEAN
WipeFile(char *);


FILELIST
*BuildList(FILELIST *List, BOOLEAN SubDirs, char *Pattern)
{
   FILELIST *Next;

   if (SubDirs)
      printf("Traversing sub-directories as well\n");

   /* Declare space for next element in linked list. */

   Next = (FILELIST *) malloc(sizeof(FILELIST));
   if (Next == NULL)
      return (FILELIST *) NULL;   /* Out of memory !! */

   /* Check to see if this is the head of the list. */

   memset(Next->fname,'\0',sizeof(Next->fname));
   strcpy(Next->fname,Pattern);
   Next->next=List;

   return Next;
}


/****************************************************************************
Check to see if next entry is a file or a directory
****************************************************************************/
int
directory(char attr)
{
    /* Simply check all combinations of hidden flags, read only flags, */
    /* system flags, etc.... to see what the entry is.                 */
    if ((attr == FA_DIREC) ||
	(attr == (FA_DIREC | FA_HIDDEN)) ||
	(attr == (FA_DIREC | FA_SYSTEM)) ||
	(attr == (FA_DIREC | FA_RDONLY)) ||
	(attr == (FA_DIREC | FA_HIDDEN | FA_SYSTEM)))
	return(TRUE);
    else
	return(FALSE);
}


void
Help()
{
   fprintf(stderr,"\nUsage: wf [-s] <file-1> <file-2> ... <file-n>\n");
   fprintf(stderr,"   Eg: wf *.dat\n       wf -s *.dat *.doc\n");
}




BOOLEAN
FileSpecMatches(char *String, char *Pattern)
{
   char         Prefix[10]     = {0},
		Suffix[4]      = {0};

   static char	PattPrefix[10] = {0},
		PattSuffix[10] = {0};

   char *p;

   int  FullstopPos    = 0;


   static BOOLEAN SplitPattern = FALSE;


   /*
    * Erase the strings.
   */

   memset(Prefix,'\0',10);
   memset(PattPrefix,'\0',4);
   memset(Suffix,'\0',10);
   memset(PattSuffix,'\0',4);

   /*
    * Split the prefix from the suffix.
   */

   p=String;
   FullstopPos=0;
   while (*p && *p != '.')
   {
      FullstopPos++;
      p++;
   }

   strncpy(Prefix,&String[0],FullstopPos);
   strncpy(Suffix,&String[FullstopPos+1],(strlen(String) - FullstopPos));

   /*
    * Now split the pattern if it hasn't already been done.
   */

   if (! SplitPattern)
   {
      p=Pattern;
      FullstopPos=0;
      while (*p && *p != '.')
      {
	 FullstopPos++;
	 p++;
      }
      strncpy(PattPrefix,&Pattern[0],FullstopPos);
      strncpy(PattSuffix,&Pattern[FullstopPos+1],(strlen(Pattern) - FullstopPos));
      SplitPattern=TRUE;
   }

   /*
    * Now check the file prefix and suffix against the pattern.
   */

   if (PatternMatches(Prefix, PattPrefix))
      if (PatternMatches(Suffix, PattSuffix))
	 return TRUE;
      else
	 return FALSE;
   else
      return FALSE;
}




BOOLEAN
PatternMatches(char *String, char *Pattern)
{
  BOOLEAN  Match;
  char     nc, np, ns, *p, *s;
  int      PattCharCount;


  /*
   * First, if the pattern is '*' then we match;
  */

  if (strlen(Pattern) == 1 && *Pattern == '*')
     return TRUE;

  /*
   * Make sure that if pattern exists but the string doesn't, then
   * they do not match. This is for things like s=crap.123 p=crap
  */

  if (strlen(String) > 0 && strlen(Pattern) == 0)
     return FALSE;

  /*
   * Now count number of real chars and '?' in the pattern. The string
   * must be at least the same length.
  */

  p=Pattern;
  PattCharCount=0;
  while (*p)
     if (*p++ != '*')
	PattCharCount++;

  if (strlen(String) < PattCharCount)
     return FALSE;


  /*
   * Now check for more general cases.
  */

  p=Pattern;
  s=String;
  Match = TRUE;

  while (*p && *s)
  {
     np=toupper(*p);
     ns=toupper(*s);

     if (np == '*')
     {
	nc = toupper(*++p);
	if (nc != '\0' && nc == ns)
	   p++;
     }
     else
     {
	if (np != '?' && (ns == '\0' || np != ns))
	   Match = FALSE;
	p++;
     }
     s++;
  }

  if (Match)
     printf("%s matches %s !!\n",Pattern, String);
  return Match;
}




BOOLEAN
SearchDir(char *Pattern)
{
   BOOLEAN done=FALSE,
	   OK;
   struct ffblk ffblk;          /* File structure                      */

   /* Get a list of all directory entries within this sub-dir */
   done=findfirst("*.*",&ffblk,255);

   /* While there are still directory entries to process ......*/
   OK=TRUE;
   while (! done)
   {
      /*
       * Never process entries if they start with a '.', as this means
       * that the entry is the current or parent directory.
      */
      if (ffblk.ff_name[0] != '.' && ! directory(ffblk.ff_attrib))
	 if (FileSpecMatches(ffblk.ff_name,Pattern))
	 {
	    printf("Filespec matched: %s  and  %s\n",Pattern,ffblk.ff_name);
	    OK=WipeFile(ffblk.ff_name);              /* Try to delete it. */
	 }

      done=findnext(&ffblk);            /* Process next directory entry. */

   }
   return OK;
}



BOOLEAN
WipeFile(char *Pattern)
{
   FILE *fp;

   char Nada[BUFSIZ];

   int  fd,
	Unlinked;

   long Buffers,
	BytesWritten,
	Chars,
	Count,
	FileSize;


   /* Initialise. */

   BytesWritten=0L;
   FileSize=0L;
   memset(Nada,'\0',BUFSIZ);

   /* Try to open a file handle and a stream for the file. */

   if ((fd=open(Pattern,O_RDWR|O_BINARY,S_IREAD|S_IWRITE)) == -1)
   {
      fprintf(stderr,"ERROR - Could not open file %s for writing\n",Pattern);
      return FALSE;
   }

   if ((fp=fdopen(fd,"r+w")) == NULL)
   {
      fprintf(stderr,"ERROR - Could not create stream for file handle\n");
      return FALSE;
   }

   /* Now get its size. */

   fseek(fp,0L,SEEK_END);
   FileSize=ftell(fp);
   fseek(fp,0L,SEEK_SET);

   /* Create buffer of crap, and calculate no. buffers to write. */

   Buffers=(long) FileSize / (long) BUFSIZ;
   Chars=(long) FileSize % (long) BUFSIZ;

   /* Now write this many buffers of junk to the file. */

   fprintf(stdout,"Erasing %-12s...",Pattern);
   for (Count=0L;Count < Buffers;Count++)
       BytesWritten += (long) write(fd,Nada,sizeof(Nada));

   /* Now write remaining number of bytes to file. */

   BytesWritten += (long) write(fd,Nada,Chars);

   /* Check status of write operation. */

   if (BytesWritten == FileSize)
      fprintf(stdout,"OK...");
   else
      fprintf(stdout,"FAILED...");

   /* Now try to unlink the file. */

   fflush(fp);
   fclose(fp);

   Unlinked=unlink(Pattern);

   if (Unlinked == 0)
      fprintf(stdout,"deleted.\n");
   else
      fprintf(stdout,"COULD NOT REMOVE\n");

   /* Decide on return code to return to calling function. */

   if (Unlinked != 0 || BytesWritten != 1)
      return FALSE;
   return TRUE;

}



int
main(int argc, char **argv)
{

   int n                   = 0,
       StartPos            = 1;

   BOOLEAN SubDirs         = FALSE,
	   Wiped           = TRUE;

   FILELIST *FileList      = (FILELIST *) NULL;

   /*
    * Check parameters. They can supply a -s flag, which means
    * to do sub-directories as well.
   */

   if ((argc < 2) ||
       ((strcmp(argv[1],"-s") == 0) && argc < 3))
   {
      Help();
      exit(1);
   }

   if (strcmp(argv[1],"-s") == 0)
   {
      SubDirs=TRUE;
      StartPos=2;
   }

   /*
    * Build a linked list of files to delete, paying attention to
    * any wildcard characters they may have provided.
   */

   for (n=StartPos;n < argc;n++)
       FileList=BuildList(FileList,SubDirs,argv[n]);

   /* For each argument, do stuff. */

   while (FileList)
   {
       Wiped=SearchDir(FileList->fname);
       FileList=FileList->next;
   }

   /* Done ! */

   if (! Wiped)
      return(1);
   return(0);

}

