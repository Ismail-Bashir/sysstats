#ifndef STATS_FUNCTIONS_H
#define STATS_FUNCTIONS_H

#include <stdio.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <utmp.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <utmpx.h>
#include <sys/resource.h>
#include <utmp.h>
#include <sys/utsname.h>
#include <math.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>


// Function prototypes
void GetInfoTop(int samples, int tdelay, int sequential, int iteration);


// void storeMemArr(int samples, int memFD[2]);
void storeMemArr(int samples, int memFD[2],int tdelay);

void fcnForPrintMemoryArr(int sequential, int samples, char memArr[][1024], int iteration,int memFD[2]);
void memoryGraphics(double virtual_used_gb, double* prev_used_gb, char memArr[][1024], int iteration);


void storeUserInfoThird(int userFD[2],int ucountFD[2]);
void printUserInfoThird(int userFD[2]);



void printCores();

void storeCpuArr(int cpuFD[2]);
void printCpuUsageAndGraphics(int cpuPFD[2], int cpuCFD[2], int sequential, int i,int graphics);
double calculateCpuUsage(unsigned long prevCpuUsage[7], unsigned long currCpuUsage[7]);
// double calculateCpuUsage(int cpuPFD[2],int cpuCFD[2]);
void setCpuGraphics(int sequential,char cpuArr[][200],float curCpuUsage,float *prevCpuUsage,int sampleIndex);



void printSystemInfoLast();
double calculateVirtUsed();

void reserve_space(int samples);

#endif // STATS_FUNCTIONS_H