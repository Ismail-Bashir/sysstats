#include "stats_functions.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <utmp.h>
#include <math.h>
#include <fcntl.h>
#include <paths.h>

// Prints the top line with memory usage and sample metadata
void GetInfoTop(int samples, int tdelay, int sequential, int i) {
    struct rusage usage_info;
    int result = getrusage(RUSAGE_SELF, &usage_info);

    if (sequential)
        printf(">>> iteration %d\n", i);
    else {
        printf("\033[H\033[2J");
        printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay);
    }

    if (result == 0)
        printf("Memory usage: %ld kilobytes\n", usage_info.ru_maxrss);
    else
        perror("Failed to get resource usage");
}

// Stores memory statistics into a pipe
void storeMemArr(int samples, int memFD[2], int tdelay) {
    char memArr[samples][1024];
    struct sysinfo sys_info;

    for (int i = 0; i < samples; i++) {
        sysinfo(&sys_info);
        double phys_total_gb = sys_info.totalram / (1024.0 * 1024 * 1024);
        double phys_free_gb = sys_info.freeram / (1024.0 * 1024 * 1024);
        double phys_used_gb = phys_total_gb - phys_free_gb;

        double swap_total_gb = sys_info.totalswap / (1024.0 * 1024 * 1024);
        double swap_free_gb = sys_info.freeswap / (1024.0 * 1024 * 1024);

        double virtual_used_gb = phys_used_gb + (swap_total_gb - swap_free_gb);
        double virtual_total_gb = phys_total_gb + swap_total_gb;

        snprintf(memArr[i], sizeof(memArr[i]),
                 "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB",
                 phys_used_gb, phys_total_gb,
                 virtual_used_gb, virtual_total_gb);

        if (write(memFD[1], memArr[i], sizeof(memArr[i])) == -1) {
            perror("Error writing memory data to pipe");
            kill(getpid(), SIGTERM);
            kill(getppid(), SIGTERM);
        }

        sleep(tdelay);
    }
}

// Displays memory information from memArr
void fcnForPrintMemoryArr(int sequential, int samples, char memArr[][1024], int i, int memFD[2]) {
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");

    if (sequential) {
        for (int k = 0; k < samples; k++) {
            if (k == i)
                printf("%s\n", memArr[k]);
            else
                printf("\n");
        }
    } else {
        for (int j = 0; j <= i; j++)
            printf("%s\n", memArr[j]);
    }
}

// Adds a graphical representation to the memory line
void memoryGraphics(double virtual_used_gb, double *prev_used_gb, char memArr[][1024], int i) {
    double difference = virtual_used_gb - *prev_used_gb;
    char graphicsStr[1024] = "|";
    char infoStr[100];

    if (i == 0 || fabs(difference) < 0.01) {
        snprintf(graphicsStr + 1, sizeof(graphicsStr) - 1,
                 "%s %.2f (%.2f)", difference >= 0 ? "o" : "@", difference, virtual_used_gb);
    } else {
        char symbol = difference < 0 ? ':' : '#';
        int count = fabs(difference) * 100;
        for (int j = 0; j < count && j < 50; j++) {
            strncat(graphicsStr, &symbol, 1);
        }
        strcat(graphicsStr, difference < 0 ? "@" : "*");
        snprintf(infoStr, sizeof(infoStr), " %.2f (%.2f)", difference, virtual_used_gb);
        strcat(graphicsStr, infoStr);
    }

    strcat(memArr[i], " ");
    strcat(memArr[i], graphicsStr);
    memArr[i][1023] = '\0';

    *prev_used_gb = virtual_used_gb;
}

// Writes user session info to pipe
void storeUserInfoThird(int userFD[2], int ucountFD[2]) {
    struct utmp *utmp;
    if (utmpname(_PATH_UTMP) == -1) {
        perror("Failed to set utmp file path");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
    }

    setutent();
    int userLine_count = 0;

    while ((utmp = getutent()) != NULL) {
        if (utmp->ut_type == USER_PROCESS) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "%s\t %s (%s)\n", utmp->ut_user, utmp->ut_line, utmp->ut_host);

            if (write(userFD[1], buffer, strlen(buffer)) == -1) {
                perror("Error writing user info to pipe");
                kill(getpid(), SIGTERM);
                kill(getppid(), SIGTERM);
            }

            userLine_count++;
        }
    }

    if (write(ucountFD[1], &userLine_count, sizeof(userLine_count)) == -1) {
        perror("Error writing user count to pipe");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
    }

    endutent();
}

// Reads and prints user info from pipe
void printUserInfoThird(int userFD[2]) {
    char buffer[1024] = "";
    ssize_t bytesRead = read(userFD[0], buffer, sizeof(buffer) - 1);

    printf("### Sessions/users ###\n");

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        printf("%s", buffer);
    } else if (bytesRead == -1) {
        perror("Error reading from user pipe");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
    }
}

// Prints number of CPU cores
void printCores() {
    int num_cpu = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of cores: %d\n", num_cpu);
}

// Gathers CPU usage data and writes to pipe
void storeCpuArr(int cpuFD[2]) {
    unsigned long currCpuUsage[7];
    FILE *fp = fopen("/proc/stat", "r");

    if (!fp) {
        perror("Failed to open /proc/stat");
        exit(EXIT_FAILURE);
    }

    if (fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu",
               &currCpuUsage[0], &currCpuUsage[1], &currCpuUsage[2],
               &currCpuUsage[3], &currCpuUsage[4], &currCpuUsage[5], &currCpuUsage[6]) != 7) {
        fprintf(stderr, "Failed to parse /proc/stat\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    fclose(fp);

    if (write(cpuFD[1], &currCpuUsage, sizeof(currCpuUsage)) == -1) {
        perror("Error writing CPU data to pipe");
        kill(getpid(), SIGTERM);
        kill(getppid(), SIGTERM);
        exit(EXIT_FAILURE);
    }
}

// Calculates CPU usage between two samples
double calculateCpuUsage(unsigned long prev[7], unsigned long curr[7]) {
    unsigned long idle_prev = prev[3] + prev[4];
    unsigned long idle_cur = curr[3] + curr[4];

    unsigned long total_prev = 0, total_cur = 0;
    for (int i = 0; i < 7; i++) {
        total_prev += prev[i];
        total_cur += curr[i];
    }

    double total_diff = total_cur - total_prev;
    double idle_diff = idle_cur - idle_prev;

    return ((total_diff - idle_diff) / total_diff) * 100.0;
}

// Prints a graphical representation of CPU usage
void setCpuGraphics(int sequential, char cpuArr[][200], float curUsage, float *prevUsage, int index) {
    int barCount = 3 + (index == 0 ? (int)curUsage : (int)(curUsage - *prevUsage));

    if (barCount < 3) barCount = 3;

    char line[200] = "         ";
    for (int i = 0; i < barCount; i++) strcat(line, "|");

    char percent[50];
    snprintf(percent, sizeof(percent), " %.2f%%", curUsage);
    strcat(line, percent);

    strcpy(cpuArr[index], line);

    for (int i = 0; i <= index; i++)
        printf("%s\n", cpuArr[i]);

    *prevUsage = curUsage;
}

// Displays system information at the end
void printSystemInfoLast() {
    struct utsname sys;
    FILE *fp = fopen("/proc/uptime", "r");

    if (!fp) {
        perror("Unable to read /proc/uptime");
        return;
    }

    double uptime;
    fscanf(fp, "%lf", &uptime);
    fclose(fp);

    int days = uptime / (24 * 3600);
    uptime -= days * 24 * 3600;
    int hours = uptime / 3600;
    uptime -= hours * 3600;
    int minutes = uptime / 60;
    int seconds = (int)uptime % 60;

    if (uname(&sys) == 0) {
        printf("### System Information ###\n");
        printf("System Name: %s\n", sys.sysname);
        printf("Machine Name: %s\n", sys.nodename);
        printf("Version: %s\n", sys.version);
        printf("Release: %s\n", sys.release);
        printf("Architecture: %s\n", sys.machine);
        printf("System running since: %d days %02d:%02d:%02d\n", days, hours, minutes, seconds);
    }
}

// Calculates the total virtual memory used
double calculateVirtUsed() {
    struct sysinfo info;
    sysinfo(&info);

    double used = (info.totalram - info.freeram + info.totalswap - info.freeswap) / (1024.0 * 1024 * 1024);
    return used;
}

// Reserves blank space in terminal output for overwrite/updating
void reserve_space(int samples) {
    for (int i = 0; i < samples + 1; i++)
        printf("\n");
}
