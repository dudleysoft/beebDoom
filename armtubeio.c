// pstnotpd 16122011 changed all _swi calls to their _swix equivalent to avoid being bumped back into the OS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "armcopro.h"
#include "swis.h"

#define TRUE        (1)
#define FALSE       (0)
#define CR          (13)
#define LF          (10)
#define UNUSED(k)       k = k

extern caddr_t _get_stack_pointer(void);

extern int main(int argc, char *argv[]);

caddr_t
    _sbrk (int incr)
{
    extern char   _end;	/* Defined by the linker.  */
    static char * heap_end;
    char *        prev_heap_end;
    caddr_t       stack = _get_stack_pointer();

    if (heap_end == 0)
	{
     	   heap_end = & _end;
        //heap_end = (char *)(0x5b000); 
	}
	
    prev_heap_end = heap_end;

    if ((heap_end + incr) > stack)
    {
       //printf("Error: heap > stack (%08x, %08x)\n", (unsigned int)(heap_end+incr), (unsigned int)stack);
	  return ((caddr_t)-1);
    }

    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

void WriteByteToIo(void* _Dst, BYTE byte)
{
  int flags = 0;
  _kernel_oserror *errptr;
    BYTE paramBlock[64];
    UINT32* beebLocation = (UINT32*)paramBlock;
    BYTE* pByte = (BYTE*)paramBlock + 4;
    int reason = 6;

    *beebLocation = (UINT32)_Dst;
    *pByte = byte;
    //    _swi(OS_Word, _INR(0,1), reason, beebLocation);
    errptr = _swix(OS_Word, _INR(0,1), reason, beebLocation);
}

BYTE ReadByteFromIo(void* _Address)
{
    int flags = 0;
    _kernel_oserror *errptr;
    BYTE paramBlock[64];
    UINT32* beebLocation = (UINT32*)paramBlock;
    BYTE* pByte = (BYTE*)paramBlock + 4;
    int reason = 5;

    *beebLocation = (UINT32)_Address;

    //    _swi(OS_Word, _INR(0,1), reason, beebLocation);
    errptr = _swix(OS_Word, _INR(0,1), reason, beebLocation);
    
    return *pByte;
}

//void* memcpy(void * _Dst, const void* _Src, size_t _Size);
void memcpyfromio_slow(void * _Dst, const void* _Src, size_t _Size)
{
    BYTE* source = (BYTE*)_Src;
    BYTE* dest = (BYTE*)_Dst;

    size_t n = 0;
    for (n = 0; n < _Size; n++)
    {
        dest[n] = ReadByteFromIo(source + n);
    }
}

//void* memcpy(void * _Dst, const void* _Src, size_t _Size);
void memcpytoio_slow(void * _Dst, const void* _Src, size_t _Size)
{
    BYTE* source = (BYTE*)_Src;
    BYTE* dest = (BYTE*)_Dst;

    size_t n = 0;
    for (n = 0; n < _Size; n++)
    {
        WriteByteToIo(dest + n, source[n]);
    }
}

void strreverse(char* begin, char* end) 
{
    char tmp;

    while(end > begin)
    {
        tmp = *end, *end-- = *begin, *begin++ = tmp;
    }
}

char* itoa(int value, char* str, int base) 
{
    static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* wstr = str;
    int sign;
    div_t res;

    // Validate base
    if (base < 2 || base > 35)
    {
        *wstr = '\0'; 
        return wstr; 
    }

    // Take care of sign
    if ((sign = value) < 0)
    {
        value = -value;
    }

    // Conversion. Number is reversed.
    do 
    {
        res = div(value, base);
        *wstr++ = num[res.rem];
    } while (value = res.quot);

    if(sign < 0)
    {
        *wstr++ = '-';
    }

    *wstr = '\0';

    // Reverse string
    strreverse(str, wstr - 1);
		
    return str;
}

int _write(int fd, const char *ptr, int len)
{
  int flags = 0;
  _kernel_oserror *errptr;
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        int         count, ch;
        static int      previous = 0;

        for (count = 0; count < len; count++)
        {
            ch = ptr[count];
            if ((ch == LF) && (previous != CR))
            {
                /* Expand isolated LF into CR+LF pair */
	      //                _swi(OS_NewLine, 0);
	      errptr = _swix(OS_NewLine, 0);
            }
            else
            {
                /* Use OS_WriteC */
	      //                _swi(OS_WriteC, _IN(0), ch);
	      errptr = _swix(OS_WriteC, _IN(0), ch);
            }
            previous = ch;
        }

        UNUSED(fd);
    }
    else
    {
#ifdef TRACE
        printf("_write()\r\n");
        printf("_write(&%x, &%x, &%x)\r\n", fd, ptr, len);
#endif
        int r0,r1,nextbufferbyte,bytesleft,filepointer;

        // _swi(OS_GBPB, _INR(0,3)|_OUTR(0,4), 2, fd, ptr, len, &r0, &r1, &nextbufferbyte, &bytesleft, &filepointer);
        errptr = _swix(OS_GBPB, _INR(0,3)|_OUTR(0,4), 2, fd, ptr, len, &r0, &r1, &nextbufferbyte, &bytesleft, &filepointer);

        return len - bytesleft;
    }

    return len;
}


void debug_print(const char *ptr, int len)
{
  int flags = 0;
  _kernel_oserror *errptr;
    if (len == 0)
    {
        len = strlen(ptr);
    }

    _write(STDOUT_FILENO, ptr, len);

    //    _swi(OS_NewLine, 0);
    errptr = _swix(OS_NewLine, 0);
}

char* argvec[128];

int __appentry()
{
  int flags = 0;
  _kernel_oserror *errptr;
    char *args;
    char commandline[128];
    UINT32 ioAddress;
    UINT32 maxmem;
    long long starttime = 0;

    // Read the command-line address from the io processor
    //    _swi(OS_Args, _INR(0,2)|_OUT(2), 1, 0, 0, &ioAddress);
    errptr = _swix(OS_Args, _INR(0,2)|_OUT(2), 1, 0, 0, &ioAddress);
    // We want all of the command-line so back up to the
    // nearest page (0x700 on B and B+ and 0xDC00 on a master)
    ioAddress = (ioAddress / 256) * 256;
    // copy the memory over from the io processor to the ARM
    memcpyfromio_slow(commandline, (void*)ioAddress, sizeof(commandline));

    args = commandline;
    int nchar = 0;
    int narg = 1;
    argvec[0] = args;

    while (args[nchar] != 0 && args[nchar] != '\r' && args[nchar] != '\n' && nchar < 128)
    {
        if (args[nchar] == ' ')
        {
            args[nchar++] = '\0';
            argvec[narg++] = &args[nchar];
        }
        else
        {
            nchar++;
        }

        // Too many arguments, so we'll stop there
        if (narg > (sizeof(argvec) / sizeof(char*)))
        {
            break; // out of the while
        }
    }
    args[nchar] = '\0';

    return main(narg, argvec);
}

int _link(const char *__path1, const char *__path2 )
{
#ifdef TRACE
    printf("_link()\r\n");
    printf("_link(\"%s\",\"%s\")\r\n", __path1, __path2);
#endif
}

int _unlink(const char *__path )
{
#ifdef TRACE
    printf("_unlink()\r\n");
    printf("_unlink(\"%s\")\r\n", __path);
#endif
    return -1;
}

int _execve(const char *__path, char * const __argv[], char * const __envp[] )
{
#ifdef TRACE
    printf("_execve()\r\n");
    printf("_execve(\"%s\")\r\n", __path);
#endif
    return -1;
}

int _read(int fd, char *ptr, int len)
{
  int flags = 0;
  _kernel_oserror *errptr;
    if (fd == STDIN_FILENO)
    {
      /*
        int         ch;
        static int      previous = 0;

        // Get new from ReadC 
        _swi(OS_ReadC, _OUT(0), &ch);
        *ptr = (char)ch;
        previous = ch;

        // Echo to stdout accounting for carriage return 
        _swi(OS_WriteC, _IN(0), ch);
        if (previous == CR) _swi(OS_WriteI + LF, 0);

        // Returns the actual number of bytes read 
        return 1;
       */

      char c;
      int  i,j;
      unsigned char *p;

      p = (unsigned char*)ptr;

      for (i = 0; i < len; i++)	{
	//	_swi(OS_ReadC, _OUT(0), &c);
	errptr = _swix(OS_ReadC, _OUT(0), &c);

	if ((int)c==127&&i>=0) {
	  --i;--i;
	  p--;
#ifdef TRACE
	  printf("i: %d p: %d\n",i,(int)p-(int)ptr);
#endif
	} else {
	  *p++ = c;
	}
	//_swi(OS_WriteC, _IN(0), c);
	errptr = _swix(OS_WriteC, _IN(0), c);

	if (c == 0x0D && i <= (len - 2)) {
	  *p = 0x0A;
	  //_swi(OS_WriteI + LF, 0);
	  errptr = _swix(OS_WriteI + LF, 0);
	  return i + 2;
	}
      }
      return i;
    }
    else
    {
#ifdef TRACE
        printf("_read()\r\n");
        printf(" _read(&%x, &%x, &%x)\r\n", fd, ptr, len);
#endif
        int r0,r1,nextbufferbyte,bytesleft,filepointer;

        //_swi(OS_GBPB, _INR(0,3)|_OUTR(0,4), 4, fd, ptr, len, &r0, &r1, &nextbufferbyte, &bytesleft, &filepointer);
        errptr = _swix(OS_GBPB, _INR(0,3)|_OUTR(0,4), 4, fd, ptr, len, &r0, &r1, &nextbufferbyte, &bytesleft, &filepointer);

#ifdef TRACE
        printf("_read_done(&%x, &%x, &%x)\r\n", nextbufferbyte, bytesleft, filepointer);
#endif
        return len - bytesleft;
    }
}

int _close(int fd)
{
  int flags = 0;
  _kernel_oserror *errptr;
    if (fd == STDERR_FILENO || fd == STDIN_FILENO || fd == STDOUT_FILENO)
    {
        return -1;
    }
#ifdef TRACE
    printf("_close()\r\n");
    printf("_close(&%x)\r\n", fd);
#endif

    //    _swi(OS_Find, _INR(0, 1), 0, fd);
    errptr = _swix(OS_Find, _INR(0, 1), 0, fd);

    // Check if all went well
    if (flags & _V) {
      printf("\nError %d : %s\n", errptr->errnum, errptr->errmess);
      return -1;
    }

    return 0;
}

int _isatty(int fd)
{
    if (fd == STDERR_FILENO || fd == STDIN_FILENO || fd == STDOUT_FILENO)
    {
        /* Yes */
        return 1;
    }
    else
    {
        return 0;
    }
}

int _fstat(int fd, struct stat *st)
{
    memset(st, 0, sizeof(struct stat));

    if (fd == STDERR_FILENO || fd == STDIN_FILENO || fd == STDOUT_FILENO)
    {
        // We're character based
        st->st_mode = S_IFCHR;
        st->st_dev = 1;
        return 0;
    }
    else
    {
#ifdef TRACE
        printf("_fstat()\r\n");
        printf("_fstat(&%x, &%x)\r\n", fd, st);
#endif
        // We're file based
        st->st_mode = (unsigned int)S_IFREG;
        st->st_dev = 0;
        st->st_nlink = 1;
    }

    st->st_size = _fgetextent(fd);

    return 0;
}

int _stat(const char *fn, struct stat *st)
{
    int fh=open(fn,O_RDONLY);
    int result = _fstat(fh,st);
    _close(fh);

    return result;
}

int _fgetextent(int fd)
{
  int flags = 0;
  _kernel_oserror *errptr;
    int r0,r1,extent;

    //_swi(OS_Args, _INR(0, 1)|_OUTR(0,2), 2, fd, &r0, &r1, &extent);
    errptr = _swix(OS_Args, _INR(0, 1)|_OUTR(0,2), 2, fd, &r0, &r1, &extent);
    return extent;
}

int _fgetpos(int fd)
{
  int flags = 0;
  _kernel_oserror *errptr;

  int r0,r1,pos;

  //  _swi(OS_Args, _INR(0, 1)|_OUTR(0,2), 0, fd, &r0, &r1, &pos);
  errptr = _swix(OS_Args, _INR(0, 1)|_OUTR(0,2), 0, fd, &r0, &r1, &pos);

  return pos;
}

void _fsetpos(int fd, int pos)
{
  int flags = 0;
  _kernel_oserror *errptr;

  //    _swi(OS_Args, _INR(0, 2), 1, fd, pos);
  errptr = _swix(OS_Args, _INR(0, 2)|_OUT(_FLAGS), 1, fd, pos, &flags);
}

int _getpid_r (struct _reent *r)
{
    return 42;
}

int _kill_r (struct _reent *r, int pid, int sig)
{
    return 0;
}

// These structs have been deprecated from <time.h> but seem to be needed
// for newlib
//struct timeval {
 // int tv_sec; /* seconds */
 // int tv_usec; /* and microseconds */
//};

int _lseek(int fd, int ptr, int dir)
{
    if (fd == STDIN_FILENO || fd == STDERR_FILENO || fd == STDOUT_FILENO)
    {
        /* Seeking ignored */
        UNUSED(fd);
        UNUSED(ptr);
        UNUSED(dir);
    }
    else
    {
#ifdef TRACE
        printf("_lseek()\r\n");
        printf("_lseek(&%x, &%x, &%x)\r\n", fd, ptr, dir);
#endif

	   /* mod to return pointer position rather than 0 */
        if (dir == SEEK_SET)
        {
            _fsetpos(fd, ptr);
		 return ptr;
        }
        else if (dir == SEEK_CUR)
        {
            int pos = _fgetpos(fd);

            _fsetpos(fd, pos + ptr);
		 return (pos+ptr);
        }
        else if (dir == SEEK_END)
        {
            int extent = _fgetextent(fd);

            _fsetpos(fd, extent - ptr);
		 return (extent - ptr);
        }
    }
    return 0;
}

int _open(const char * filename, int mode, ...)
{
  int flags = 0;
  _kernel_oserror *errptr;
#ifdef TRACE
    printf("_open()\r\n");
    printf("_open(\"%s\", &%x)\r\n", filename, mode);
#endif
    int reason;
    int handle;

    if ((mode &7)== O_WRONLY)
    {
        reason = 0x80;
    }
    else if ((mode & 7) == O_RDONLY)
    {
        reason = 0x40;
    }
    else if ((mode & 7) == O_RDWR)
    {
        reason = 0xC0;
    }

    //    _swi(OS_Find, _INR(0, 1) | _OUT(0), reason, filename, &handle);
    errptr = _swix(OS_Find, _INR(0, 1) | _OUT(0)|_OUT(_FLAGS), reason, filename, &handle, &flags);

    // Check if all went well
    if (flags & _V) {
    //   printf("\nError %d : %s\n", errptr->errnum, errptr->errmess);
      return -1;
    }

    if (handle == 0)
    {
        return -1;
    }

    if (mode & O_APPEND)
    {
        _lseek(handle, 0, SEEK_END);
    }
    return handle;
}

struct timezone {
  int tz_minuteswest; // Number of minutes wet of UTC
  int tz_dsttime; // If nonzero DST applies
};

int _gettimeofday_r (struct timeval * tp, struct timezone * tzp)
{
   return 0;
}

unsigned char dirCache[4096];

DIR dirEntCache;

DIR *opendir (const char *path)
{
    //printf("opendir: %s\n",path);
    char pathBuffer[256];
    if (path)
    {
        sprintf(pathBuffer,"DIR %s\n",path);
        _swi(OS_CLI, _IN(0),pathBuffer);
    }
    int r0,r1,r2,read,nextOffset;
    int id = 0;
    _swi(OS_GBPB, _INR(0,4)|_OUTR(3,4),8,pathBuffer,dirCache,77,0,&read,&nextOffset);

    unsigned char *ptr = &dirCache[0];

    unsigned char buffer[32];

    while(read  < 77)
    {
        int len = *ptr++;
        dirEntCache.__d_dirent[id].d_type=S_IFREG;
        strncpy(dirEntCache.__d_dirent[id].d_name,ptr,len);
        ptr+=len;
        while(isspace(dirEntCache.__d_dirent[id].d_name[len-1])) len--;
        dirEntCache.__d_dirent[id].d_name[len]=0;
        id++;
        read++;
    }

    strcpy(dirEntCache.__d_dirname,path);
    dirEntCache.__d_position=-1;
    dirEntCache.__d_cookie=id;

    return &dirEntCache;
}

DIR *fdopendir (int fd)
{
    return opendir(NULL);
}

struct dirent *readdir (DIR *dir)
{
    if (dir->__d_position == dir->__d_cookie)
    {
        return NULL;
    }
    dir->__d_position++;
    return &dir->__d_dirent[dir->__d_position];
}

int readdir_r (DIR * __restrict dir, struct dirent * __restrict in, struct dirent ** __restrict out)
{
    return 0;
}

void rewinddir (DIR *dir)
{
    dir->__d_position = -1;
}
int closedir (DIR *dir)
{
    dir->__d_cookie = -1;
    dir->__d_position = -1;
}

char dirBuffer[256];

const char *getcwd(char *buffer,int maxlen)
{
    char pathBuffer[24];
    _swi(OS_GBPB, _INR(0,4),6,pathBuffer,pathBuffer,24,24);
    int offset=pathBuffer[0]+1;
    int drive = pathBuffer[1];
    if (pathBuffer[offset+1]=='$')
    {
        sprintf(buffer,":%c.$",drive);
        return buffer;
    }
    int len = offset+pathBuffer[offset]+1;
    pathBuffer[len--]=0;
    while(isspace(pathBuffer[len]))
    {
        buffer[len--]=0;
    }
    _swi(OS_CLI,_IN(0),"DIR ^");
    getcwd(buffer,maxlen);
    char *ptr = &buffer[strlen(buffer)];
    *ptr++='.';
    *ptr=0;
    strcat(buffer,&pathBuffer[offset+1]);
    len = strlen(buffer);
    while(isspace(buffer[len-1])) buffer[--len]=0;
    sprintf(dirBuffer,"DIR %s",buffer);
    _swi(OS_CLI,_IN(0),dirBuffer);
    return buffer;
}

char chdirBuffer[256];
char chdirWork[256];

int chdir(const char *dir)
{
    int flags;
    // If we're a root directory
    if (dir[0]==':' || dir[0]=='$')
    {
        sprintf(chdirBuffer,"DIR %s",dir);
        _swix(OS_CLI,_IN(0)|_OUT(_FLAGS),chdirBuffer,&flags);    
    }
    else
    {
        sprintf(chdirBuffer,"DIR @.%s",dir);
        _swix(OS_CLI,_IN(0)|_OUT(_FLAGS),chdirBuffer,&flags);
    }
    return 0;
}

size_t strlen(const char *src)
{
    size_t l=0;
    while(*src++)
    {
        l++;
    }
    return l;
}

char *strdup(const char *src)
{
    int len = strlen(src)+1;
    char *res=malloc(len);
    if (res)
    {
        strcpy(res,src);
    }
    return res;
}

int access(const char *src,int amode)
{
    FILE *fhand = fopen(src,"r");
    if (fhand)
    {
        fclose(fhand);
        return 0;
    }
    return -1;
}
