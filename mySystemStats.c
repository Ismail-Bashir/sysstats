#define _POSIX_C_SOURCE 200809L

#include "stats_functions.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <stdio.h>

pid_t memPID, userPID, cpuPID;

void ignoreCtrlZ() {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_IGN;

    if (sigaction(SIGTSTP, &action, NULL) == -1) {
        perror("Unable to ignore SIGTSTP");
        exit(EXIT_FAILURE);
    }
}

void childIgnoreSigInt() {
    signal(SIGINT, SIG_IGN);
}

void handleSigInt(int signal) {
    char input[10];

    if (signal == SIGINT) {
        printf("\nCtrl-C detected. Terminate? (y/yes or n/no): ");
        scanf(" %9s", input);

        if (strcasecmp(input, "y") == 0 || strcasecmp(input, "yes") == 0) {
            kill(memPID, SIGTERM);
            kill(userPID, SIGTERM);
            kill(cpuPID, SIGTERM);

            waitpid(memPID, NULL, 0);
            waitpid(userPID, NULL, 0);
            waitpid(cpuPID, NULL, 0);

            exit(EXIT_SUCCESS);
        } else {
            printf("Continuing...\n");
        }
    }
}

void setupSignals() {
    struct sigaction act;
    act.sa_handler = handleSigInt;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) == -1 ||
        sigaction(SIGTSTP, &act, NULL) == -1 ||
        sigaction(SIGSTOP, &act, NULL) == -1) {
        perror("Error setting up signal handlers");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    ignoreCtrlZ();

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

    int memFD[2], userFD[2], cpuPFD[2], cpuCFD[2], ucountFD[2];
    if (pipe(memFD) == -1 || pipe(userFD) == -1 || pipe(cpuPFD) == -1 ||
        pipe(cpuCFD) == -1 || pipe(ucountFD) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    if ((memPID = fork()) == 0) {
        childIgnoreSigInt();
        close(memFD[0]);
        close(userFD[0]); close(userFD[1]);
        close(cpuPFD[0]); close(cpuPFD[1]);
        close(cpuCFD[0]); close(cpuCFD[1]);
        close(ucountFD[0]); close(ucountFD[1]);

        storeMemArr(samples, memFD, tdelay);
        close(memFD[1]);
        exit(0);
    }

    if ((userPID = fork()) == 0) {
        childIgnoreSigInt();
        close(userFD[0]);
        close(memFD[0]); close(memFD[1]);
        close(cpuPFD[0]); close(cpuPFD[1]);
        close(cpuCFD[0]); close(cpuCFD[1]);
        close(ucountFD[0]);

        storeUserInfoThird(userFD, ucountFD);
        close(userFD[1]);
        close(ucountFD[1]);
        exit(0);
    }

    if ((cpuPID = fork()) == 0) {
        childIgnoreSigInt();
        close(cpuPFD[0]);
        close(cpuCFD[0]);
        close(memFD[0]); close(memFD[1]);
        close(userFD[0]); close(userFD[1]);
        close(ucountFD[0]); close(ucountFD[1]);

        for (int i = 0; i < samples; i++) {
            storeCpuArr(cpuPFD);
            sleep(tdelay);
            storeCpuArr(cpuCFD);
        }
        close(cpuPFD[1]);
        close(cpuCFD[1]);
        exit(0);
    }

    // Parent process
    close(memFD[1]); close(userFD[1]); close(cpuPFD[1]); close(cpuCFD[1]); close(ucountFD[1]);

    setupSignals();

    int userLineCount = 0;
    read(ucountFD[0], &userLineCount, sizeof(userLineCount));

    char memArr[samples][1024];
    char cpuArr[samples][200];
    double prevVirt = 0.0, currVirt = 0.0;
    float cpuUsage = 0.0f, prevCpuUsage = 0.0f;

    for (int i = 0; i < samples; i++) {
        sleep(tdelay);
        GetInfoTop(samples, tdelay, sequential, i);

        if (read(memFD[0], memArr[i], sizeof(memArr[i])) > 0) {
            if (graphics) {
                currVirt = calculateVirtUsed();
                memoryGraphics(currVirt, &prevVirt, memArr, i);
            }
        }

        if (read(cpuPFD[0], &prevCpuUsage, sizeof(prevCpuUsage)) > 0 &&
            read(cpuCFD[0], &cpuUsage, sizeof(cpuUsage)) > 0) {
            float usage = calculateCpuUsage(&prevCpuUsage, &cpuUsage);
            printf("Total CPU Usage: %.2f%%\n", usage);
            if (graphics)
                setCpuGraphics(sequential, cpuArr, usage, &prevCpuUsage, i);
        }

        if (showSystem || (!showUser && !showSystem)) {
            fcnForPrintMemoryArr(sequential, samples, memArr, i, memFD);
            printf("---------------------------------------\n");

            if (showUser || (!showUser && !showSystem)) {
                printUserInfoThird(userFD);
                printf("---------------------------------------\n");
            }

            printCores();
        } else {
            printUserInfoThird(userFD);
        }
    }

    close(memFD[0]); close(userFD[0]);
    close(cpuPFD[0]); close(cpuCFD[0]); close(ucountFD[0]);

    printf("------------------------------------\n");
    printSystemInfoLast();
    printf("------------------------------------\n");

    return 0;
}
