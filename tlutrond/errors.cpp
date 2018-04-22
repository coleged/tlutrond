/********

errors.cpp
 
 TODO:  Large parts of this are not used. This is legacy code picked up in the
 evolution of this project and need to to be weeded out.

********/
#include "lutrond.h"
#include "externals.h"

#include <stdarg.h>
#include "error_functions.h"
#include "tlpi_hdr.h"
#include "ename.c.inc"          /* Defines ename and MAX_ENAME */

#ifdef __GNUC__                 /* Prevent 'gcc -Wall' complaining  */
__attribute__ ((__noreturn__))  /* if we call this function as last */
#endif                          /* statement in a non-void function */

int        daemon_proc;        /* set nonzero by daemon_init() */


static void    err_doit(int, int, const char *, va_list);

//************   error
void
error(const char *msg)
{
    logMessage("ERROR:%s",msg);
    perror(msg);
    exit(EXIT_FAILURE);
}//perror


//************   usageError
void
usageError(const char *prog)
{
    fprintf(stderr,"\nUsage:\n %s [options]\n",MY_NAME);
    fprintf(stderr,"\nOptions:\n -c file\t\tread configuration file \"file\"\n");
    fprintf(stderr," -p port\t\tlisten on port number \"port\"\n");
    fprintf(stderr," -d\t\t\trun as a daemon\n");
    fprintf(stderr," -D\t\t\tdebug mode\n");
    fprintf(stderr," -t\t\t\ttest mode - do not connect to Lutron\n");
    fprintf(stderr," -v\t\t\tprints version and exits\n");
    exit(EXIT_FAILURE);
}//usageError


char *lerror(int number){

static const char *msg[] = {
                "",
                "Parameter count mismatch",     //1
                "Object does not exist",        //2
                "Invalid action number",        //3
                "Parameter data out of range",  //4
                "Parameter data malformed",     //5
                "Unsupported Command"           //6
             };

return((char *)msg[number]);


} // lerror

void printLerrors(){
    int i;
    // TODO left here to remind me that we need to parse Lutron error messages
    if(flag.debug){ // prints lutron error table
        for(i=1;i<7;i++){
            printf("%s\n",(const char *)lerror(i));
        }//for
    }// if debug
}

void
err_ret(const char *fmt, ...)
{
    va_list        ap;
    
    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to system call
 * Print message and terminate */

void
err_sys(const char *fmt, ...)
{
    va_list        ap;
    
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void
err_dump(const char *fmt, ...)
{
    va_list        ap;
    
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort();        /* dump core and terminate */
    exit(1);        /* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void
err_msg(const char *fmt, ...)
{
    va_list        ap;
    
    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void
err_quit(const char *fmt, ...)
{
    va_list        ap;
    
    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    int    errno_save, n;
    char    buf[MAXLINE + 1];
    
    errno_save = errno;        /* value caller might want printed */
#ifdef    HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);    /* safe */
#else
    vsprintf(buf, fmt, ap);                    /* not safe */
#endif
    n = (int)strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");
    
    if (daemon_proc) {
        syslog(level,"%s", buf);
    } else {
        fflush(stdout);        /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}

static void
terminate(bool useExit3)
{
    char *s;
    
    /* Dump core if EF_DUMPCORE environment variable is defined and
     is a nonempty string; otherwise call exit(3) or _exit(2),
     depending on the value of 'useExit3'. */
    
    s = getenv("EF_DUMPCORE");
    
    if (s != NULL && *s != '\0')
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

/* Diagnose 'errno' error by:
 
 * outputting a string containing the error name (if available
 in 'ename' array) corresponding to the value in 'err', along
 with the corresponding error message from strerror(), and
 
 * outputting the caller-supplied error message specified in
 'format' and 'ap'. */

//****  COMMENTED OUT DUE TO C++ COMPILER ERRORS FAILING TO BUILD
void
outputError(bool useErr, int err, bool flushStdout,
            const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];
    
    vsnprintf(userMsg, BUF_SIZE, format, ap);
    
    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]",
                 (err > 0 && err <= MAX_ENAME) ?
                 ename[err] : "?UNKNOWN?", strerror(err));
    else
        snprintf(errText, BUF_SIZE, ":");
    
    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);
    
    if (flushStdout)
        fflush(stdout);       //Flush any pending stdout
    fputs(buf, stderr);
    fflush(stderr);           // In case stderr is not line-buffered
}

// Display error message including 'errno' diagnostic, and return to caller

void
errMsg(const char *format, ...)
{
    va_list argList;
    int savedErrno;
    
    savedErrno = errno;       // In case we change it here
    
    va_start(argList, format);
    outputError(true, (int)errno, true, (char *)format, argList);
    va_end(argList);
    
    errno = savedErrno;
}

///COMMENTED OUT DUE TO C++ COMPILER ERRORS FAILING TO BUILD ****/

/* Display error message including 'errno' diagnostic, and
 terminate the process */

void
errExit(const char *format, ...)
{
    va_list argList;
    
    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);
    
    terminate(TRUE);
}

/* Display error message including 'errno' diagnostic, and
 terminate the process by calling _exit().
 
 The relationship between this function and errExit() is analogous
 to that between _exit(2) and exit(3): unlike errExit(), this
 function does not flush stdout and calls _exit(2) to terminate the
 process (rather than exit(3), which would cause exit handlers to be
 invoked).
 
 These differences make this function especially useful in a library
 function that creates a child process that must then terminate
 because of an error: the child must terminate without flushing
 stdio buffers that were partially filled by the caller and without
 invoking exit handlers that were established by the caller. */

void
err_exit(const char *format, ...)
{
    va_list argList;
    
    va_start(argList, format);
    outputError(TRUE, errno, FALSE, format, argList);
    va_end(argList);
    
    terminate(FALSE);
}

/* The following function does the same as errExit(), but expects
 the error number in 'errnum' */

void
errExitEN(int errnum, const char *format, ...)
{
    va_list argList;
    
    va_start(argList, format);
    outputError(TRUE, errnum, TRUE, format, argList);
    va_end(argList);
    
    terminate(TRUE);
}

/* Print an error message (without an 'errno' diagnostic) */

void
fatal(const char *format, ...)
{
    va_list argList;
    
    va_start(argList, format);
    outputError(FALSE, 0, TRUE, format, argList);
    va_end(argList);
    
    terminate(TRUE);
}

/* Print a command usage error message and terminate the process */

void
usageErr(const char *format, ...)
{
    va_list argList;
    
    fflush(stdout);           /* Flush any pending stdout */
    
    fprintf(stderr, "Usage: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);
    
    fflush(stderr);           /* In case stderr is not line-buffered */
    exit(EXIT_FAILURE);
}

/* Diagnose an error in command-line arguments and
 terminate the process */

void
cmdLineErr(const char *format, ...)
{
    va_list argList;
    
    fflush(stdout);           /* Flush any pending stdout */
    
    fprintf(stderr, "Command-line usage error: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);
    
    fflush(stderr);           /* In case stderr is not line-buffered */
    exit(EXIT_FAILURE);
}
