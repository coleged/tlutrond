/*************************************************************************\
 *                  Copyright (C) Ed Cole 2018.                            *
 *                       colege@gmail.com                                  *
 *                                                                         *
 * This program is free software. You may use, modify, and redistribute it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation, either version 3 or (at your option) any      *
 * later version. This program is distributed without any warranty.  See   *
 * the file COPYING.gpl-v3 for details.                                    *
 *                                                                         *
 \*************************************************************************/

/**********
 
 signals.cpp
 
 lutrond V4.0 April 2018
 
 sigchldHandler(), sighupHandler(), lutkill()
 
 ***********/


#include "lutrond.h"
#include "externals.h"


#define CMD_LINE_LEN 256




//************   sigchldHandler
void
sigchldHandler(int sig)
{
    
    int status;
    
    if(flag.debug) printf("SIGCHLD trapped (handler)\n"); // UNSAFE
    logMessage("SIGCHLD received");
    while( waitpid(-1,&status, WNOHANG) > 0){}; // waits for child to die
    flag.connected = false; // force lutron_connect thread to exit break out of
                            // connected loop.
    
    
}//sigchldHandler

//************   sighupHandler
void
sighupHandler(int sig)
{
    
    // NOTE: xcode catched SIGHUP for debuging.
    
    // all we do here is kill telnet and then let the SIGCHLD handler do the
    // rest when it catches this.
    
    if(flag.debug) printf("SIGHUP trapped (handler)\n"); // UNSAFE
    logMessage("SIGHUP received");
    killTelnet();
    flag.dump=true; // cause db to be dumped
    
    
    
    
    
}//sighupHandler



int lutkill(const char *pid_filename) {  // -k routine
    
    static FILE *pidfp;
    int pid;
    char command_line[CMD_LINE_LEN];
    char **exec_args;        // command line args for respawn
    
    pidfp = fopen(pid_filename, "r");
    
    if (pidfp == NULL){
        return (EXIT_FAILURE);
    };
    
    // the pid file has
    // Line 1: running lutrond process ID
    // Line 2: the command line used to invoke it
    
    if(fscanf(pidfp,"%d\n%[^\n]s",&pid,command_line) == -1){
        return (EXIT_FAILURE);
    }
    if (getpgid(pid) >= 0){ // crafty way to see if process exists
        
        if (kill(pid, SIGHUP) == -1) {
            return (EXIT_FAILURE);
        }
        
        return (EXIT_SUCCESS);
    }// no such process so we should exec another
    
    // exec a new lutrond.
    
    // convert string version of command line to array of pointers to strings
    exec_args = strarg(command_line);
    // strip any path head off the first arg (as in argv[0])
    exec_args[0] = basename(exec_args[0]);
    // We assume the process will daemonize per the -d
    // flag that will enevitably be found in command_line
    // TODO a more complete solution would be to fork/exec/die
    // to disconnect this instance as we respawn
    execvp("/usr/local/bin/lutrond",(char **)exec_args);
    
    // exec failed!
    
    logMessage("lutrond -k failed to exec process");
    
    return (EXIT_FAILURE); // if we get here its proper broke
    
}

void killTelnet(){
    
    if (getpgid(telnet_pid) >= 0){  // crafty way to see if process exists
        kill(telnet_pid,SIGTERM);    // the forked session. Will terminate and
        // raise a SIGCHLD, which will cause
        // lutron_tid2 to end and be recreated by
        // lutron_tid
        if(flag.debug) fprintf(stderr,"main1:SIGHUP sent to telnet\n");
    }else{                          // re-thread telnet
        if(flag.debug) fprintf(stderr,"main1:telnet not running\n");
        pthread_kill(lutron_tid2,SIGTERM);  // kill lutron_tid2, the thread that
        // forked telnet
        // when lutron_tid2 dies, lutron_tid will start another one
        
        if(flag.debug) fprintf(stderr,"main2:Lutron thread killed successfully\n");
        // TODO .. we don't know this for sure as we havn't tested the
        // return value of pthread_kill
    }
    
}

