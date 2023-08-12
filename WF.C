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
#include <sys\stat.h>

#define TRUE		1
#define FALSE		0

typedef int BOOLEAN;

typedef struct file_tag
{
   char fname[13];
   struct file_tag *next;
} FILELIST;



void
Help(void);


BOOLEAN
WipeFile(char *);

FILELIST
*BuildList(FILELIST *, BOOLEAN, char *);



FILELIST
*BuildList(FILELIST *List, BOOLEAN SubDirs, char *Pattern)
{
   FILELIST *Next;

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



void
Help()
{
   fprintf(stderr,"\nUsage: wf [-s] <file-1> <file-2> ... <file-n>\n");
   fprintf(stderr,"   Eg: wf *.dat\n       wf -s *.dat *.doc\n");
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
       Wiped=WipeFile(FileList->fname);
       FileList=FileList->next;
   }

   /* Done ! */

   if (! Wiped)
      return(1);
   return(0);

}




