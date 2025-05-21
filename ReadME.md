ConcurrentSystemStats:(mySystemStats is main file):
Overview


MySystemStats is a dynamic, real-time system monitoring tool that captures and displays vital statistics like CPU usage, memory utilization, and user sessions. Utilizing concurrent programming techniques, the program efficiently collects system metrics through separate child processes, ensuring a seamless and interactive user experience. This approach not only enhances performance but also provides a modular structure for the monitoring tool.
Problem Solving Approach

To tackle the complexities of concurrent data collection and presentation, the MySystemStats program employs a multi-process strategy:

    Concurrent Data Collection: By spawning distinct child processes for each type of system statistic, the program achieves non-blocking and simultaneous data collection. This method allows the program to monitor various system metrics without delay or interference between tasks.
    Inter-process Communication (IPC): IPC mechanisms facilitate the transfer of collected data from child processes back to the parent process. This centralized aggregation enables the parent process to effectively manage and display the gathered statistics.
    Graceful Signal Handling: The program includes robust signal handling to manage user interruptions (e.g., stopping the program) and ensure that the system maintains a controlled flow, enhancing usability and reliability.

Key Functions
Child Processes Functions

These functions are responsible for gathering system statistics and writing them to corresponding file descriptors for IPC:

    void storeMemArr(int samples, int memFD[2], int tdelay);
        Collects memory usage statistics and writes them to a pipe.
    void storeUserInfoThird(int userFD[2], int ucountFD[2]);
        Gathers user session information and writes it to a pipe.
    void storeCpuArr(int cpuFD[2]);
        Retrieves CPU usage data and writes it to a pipe.

Parent Processes Functions

These functions read the collected data from child processes and display it:

    void fcnForPrintMemoryArr(int sequential, int samples, char memArr[][1024], int iteration, int memFD[2]);
        Reads memory usage data from a pipe and displays it.
    void printUserInfoThird(int userFD[2]);
        Reads user session information from a pipe and displays it.


Signal Handler Function

void signal_handler(int signal) { ... }

    Purpose: This function is designed to handle signals sent to the program. It distinguishes between different signals using conditional statements and executes specific blocks of code depending on the signal received.
    Parameters: The signal parameter indicates the type of signal received by the handler.

Handling Specific Signals

    SIGINT (Ctrl-C): This signal is sent to a program when the user presses Ctrl-C. The program responds by prompting the user to confirm if they want to quit. If the user inputs 'y' or 'yes', the program kills child processes (memPID, userPID, cpuPID) using kill(), waits for them to terminate with waitpid(), and then exits successfully.

    SIGTSTP (Ctrl-Z): This signal is sent to a program when the user presses Ctrl-Z. In the given code, the signal handler simply returns upon receiving SIGTSTP, which would normally suspend the program. However, further down in the code, SIGTSTP is explicitly ignored using signal(SIGTSTP, SIG_IGN);. This means that pressing Ctrl-Z will not suspend the program, contrary to typical terminal behavior.

Compilation and Execution
Compiling the Program

Given the provided Makefile, compiling the MySystemStats program is straightforward. Simply navigate to the directory containing the source code and Makefile, and execute the following command in your terminal:

bash

make

This command will compile the source files and link them into an executable named mySystemStats based on the instructions defined in the Makefile.
Running the Program

To run the MySystemStats program, use the following command:

bash

./mySystemStats [flags...]

You can specify flags to control various aspects of the program's operation, such as the number of samples to collect or the delay between samples. The flags are optional; without them, the program will run with default settings.
Documentation

Each function in the MySystemStats program is thoroughly documented to describe its purpose, parameters, and the logic behind its implementation. This documentation aims to provide a clear understanding of how the program operates internally and how it manages to provide real-time system statistics efficiently.

For additional information about the program's functionality or how to extend it, please refer to the inline comments within the source code files.
