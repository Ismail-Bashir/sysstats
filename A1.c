#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <utmp.h>
#include <getopt.h>

#define MAX_SAMPLES 1024

// Function declarations
void printTopInfo(int samples, int tdelay, int sequential, int iteration);
void storeMemArr(char memArr[][1024], int index);
int printUserInfo();
void printSystemInfo();
void printCpuCores();
void displayMemoryGraphics(double virtUsed, double *prevUsed, char memArr[][1024], int index);
void displayMemoryArray(int sequential, int samples, char memArr[][1024], int index);
void reserveTerminalSpace(int samples);
void storeCpuStats(unsigned long usage[7]);
double calculateCpuUsage(unsigned long prev[7], unsigned long curr[7]);
double getVirtualMemoryUsed();
void displayCpuGraphics(int sequential, char cpuArr[][200], int *barCount, float curUsage, float *prevUsage, int index);

int main(int argc, char *argv[]) {
    int samples = 10, tdelay = 1;
    int showUser = 0, showSystem = 0, sequential = 0, graphics = 0;

    struct option options[] = {
        {"system", no_argument, 0, 's'},
        {"user", no_argument, 0, 'u'},
        {"graphics", no_argument, 0, 'g'},
        {"sequential", no_argument, 0, 'a'},
        {"samples", optional_argument, 0, 'b'},
        {"tdelay", optional_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "sugab::c::", options, NULL)) != -1) {
        switch (opt) {
            case 's': showSystem = 1; break;
            case 'u': showUser = 1; break;
            case 'g': graphics = 1; break;
            case 'a': sequential = 1; break;
            case 'b': if (optarg) samples = atoi(optarg); break;
            case 'c': if (optarg) tdelay = atoi(optarg); break;
        }
    }

    for (int i = optind, count = 0; i < argc && count < 2; i++, count++) {
        if (count == 0) samples = atoi(argv[i]);
        if (count == 1) tdelay = atoi(argv[i]);
    }

    unsigned long prevCpu[7], currCpu[7];
    double virtUsed = 0.0, prevVirtUsed = 0.0;
    float cpuUsage = 0.0f, prevCpuUsage = 0.0f;
    int defaultBars = 3;

    char memArr[MAX_SAMPLES][1024] = {0};
    char cpuArr[MAX_SAMPLES][200] = {0};

    for (int i = 0; i < samples; i++) {
        storeCpuStats(prevCpu);
        sleep(tdelay);
        printTopInfo(samples, tdelay, sequential, i);

        if (showSystem || (!showUser && !showSystem)) {
            storeMemArr(memArr, i);
            virtUsed = getVirtualMemoryUsed();
            if (graphics) displayMemoryGraphics(virtUsed, &prevVirtUsed, memArr, i);
            displayMemoryArray(sequential, samples, memArr, i);
            printf("---------------------------------------\n");
            printCpuCores();

            storeCpuStats(currCpu);
            cpuUsage = calculateCpuUsage(prevCpu, currCpu);
            printf("Total CPU usage: %.2f%%\n", cpuUsage);
            if (graphics) displayCpuGraphics(sequential, cpuArr, &defaultBars, cpuUsage, &prevCpuUsage, i);
        }

        if (showUser) {
            printf("---------------------------------------\n");
            printUserInfo();
        }

        if (!sequential) printf("\033[H\033[2J"); // clear terminal for non-sequential
    }

    printf("------------------------------------\n");
    printSystemInfo();
    printf("------------------------------------\n");

    return 0;
}

// Utility function definitions

void printTopInfo(int samples, int tdelay, int sequential, int iteration) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    if (sequential)
        printf(">>> Iteration %d\n", iteration);
    else
        printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay);

    printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
}

void storeMemArr(char memArr[][1024], int i) {
    struct sysinfo info;
    sysinfo(&info);

    double total = info.totalram / (1024.0 * 1024 * 1024);
    double free = info.freeram / (1024.0 * 1024 * 1024);
    double used = total - free;

    double swap_total = info.totalswap / (1024.0 * 1024 * 1024);
    double swap_free = info.freeswap / (1024.0 * 1024 * 1024);
    double virt_used = used + (swap_total - swap_free);
    double virt_total = total + swap_total;

    snprintf(memArr[i], 1024, "%.2f GB / %.2f GB -- %.2f GB / %.2f GB", used, total, virt_used, virt_total);
}

void displayMemoryArray(int sequential, int samples, char memArr[][1024], int i) {
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
    if (sequential) {
        for (int k = 0; k < samples; k++) {
            if (k == i) printf("%s\n", memArr[k]);
            else printf("\n");
        }
    } else {
        for (int j = 0; j <= i; j++) printf("%s\n", memArr[j]);
    }
}

void displayMemoryGraphics(double virtUsed, double *prevUsed, char memArr[][1024], int i) {
    double diff = virtUsed - *prevUsed;
    char line[1024] = "|";
    char symbol = diff < 0 ? ':' : '#';
    int count = (int)(fabs(diff) * 100);

    for (int j = 0; j < count && j < 50; j++) strncat(line, &symbol, 1);
    strcat(line, diff < 0 ? "@" : "*");

    char info[64];
    snprintf(info, sizeof(info), " %.2f (%.2f)", diff, virtUsed);
    strcat(line, info);

    strncat(memArr[i], " ", 1);
    strncat(memArr[i], line, sizeof(memArr[i]) - strlen(memArr[i]) - 1);
    *prevUsed = virtUsed;
}

void storeCpuStats(unsigned long usage[7]) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp || fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu",
        &usage[0], &usage[1], &usage[2], &usage[3], &usage[4], &usage[5], &usage[6]) != 7) {
        perror("Failed to read /proc/stat");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

double calculateCpuUsage(unsigned long prev[7], unsigned long curr[7]) {
    unsigned long idle_prev = prev[3] + prev[4];
    unsigned long idle_curr = curr[3] + curr[4];

    unsigned long total_prev = 0, total_curr = 0;
    for (int i = 0; i < 7; i++) {
        total_prev += prev[i];
        total_curr += curr[i];
    }

    double total_diff = total_curr - total_prev;
    double idle_diff = idle_curr - idle_prev;

    return ((total_diff - idle_diff) / total_diff) * 100.0;
}

double getVirtualMemoryUsed() {
    struct sysinfo info;
    sysinfo(&info);

    double used = ((info.totalram - info.freeram) + (info.totalswap - info.freeswap)) / (1024.0 * 1024 * 1024);
    return used;
}

void displayCpuGraphics(int sequential, char cpuArr[][200], int *barCount, float curUsage, float *prevUsage, int index) {
    int diff = (int)(curUsage - *prevUsage);
    *barCount += (index == 0) ? (int)curUsage : diff;
    if (*barCount < 3) *barCount = 3;

    char line[200] = "         ";
    for (int i = 0; i < *barCount; i++) strcat(line, "|");

    char percent[50];
    snprintf(percent, sizeof(percent), " %.2f%%", curUsage);
    strcat(line, percent);

    strcpy(cpuArr[index], line);
    for (int j = 0; j <= index; j++) printf("%s\n", cpuArr[j]);

    *prevUsage = curUsage;
}

void reserveTerminalSpace(int samples) {
    for (int i = 0; i < samples + 1; i++) printf("\n");
}

int printUserInfo() {
    setutent();
    struct utmp *entry;
    int count = 0;

    printf("### Sessions/users ###\n");
    while ((entry = getutent()) != NULL) {
        if (entry->ut_type == USER_PROCESS) {
            printf("%s\t%s", entry->ut_user, entry->ut_line);
            if (*entry->ut_host) printf(" (%s)", entry->ut_host);
            printf("\n");
            count++;
        }
    }

    endutent();
    return count;
}

void printCpuCores() {
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of cores: %d\n", cores);
}

void printSystemInfo() {
    struct utsname sys;
    double uptime_sec;

    FILE *fp = fopen("/proc/uptime", "r");
    if (fp == NULL || fscanf(fp, "%lf", &uptime_sec) != 1) {
        perror("Error reading uptime");
        return;
    }
    fclose(fp);

    int days = uptime_sec / (24 * 3600);
    uptime_sec -= days * 24 * 3600;
    int hours = uptime_sec / 3600;
    uptime_sec -= hours * 3600;
    int minutes = uptime_sec / 60;
    int seconds = (int)uptime_sec % 60;

    if (uname(&sys) == 0) {
        printf("### System Information ###\n");
        printf("System Name: %s\n", sys.sysname);
        printf("Machine Name: %s\n", sys.nodename);
        printf("Version: %s\n", sys.version);
        printf("Release: %s\n", sys.release);
        printf("Architecture: %s\n", sys.machine);
        printf("Uptime: %d days %02d:%02d:%02d\n", days, hours, minutes, seconds);
    }
}
