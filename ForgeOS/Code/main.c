#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <pwd.h>

/* ============================================================
 * PRESENTATION-ONLY UI HELPERS
 *
 * Colors, box drawing, aligned fields and cosmetic progress
 * bars. These helpers only change how output looks. They do
 * not implement or alter any functionality.
 * ============================================================ */

#define UI_WIDTH 58

#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"
#define COLOR_DIM    "\033[2m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_GRAY   "\033[90m"
#define COLOR_WHITE  "\033[97m"
#define COLOR_TITLE  "\033[1;96m"
#define COLOR_ACCENT "\033[1;36m"
#define COLOR_VALUE  "\033[1;97m"

/*
 * Colors are only emitted when stdout is a real terminal,
 * so the program stays fully usable without color support.
 */
static int uiColorsEnabled = -1;

static const char *uiC(const char *code)
{
    if (uiColorsEnabled < 0)
    {
        uiColorsEnabled = isatty(STDOUT_FILENO) ? 1 : 0;
    }

    return uiColorsEnabled ? code : "";
}

/*
 * Visible width of a string: ANSI escape sequences are
 * skipped and UTF-8 continuation bytes are not counted.
 */
static int uiVisibleLength(const char *text)
{
    int length = 0;

    while (*text != '\0')
    {
        if (*text == '\033')
        {
            while (*text != '\0' && *text != 'm')
            {
                text++;
            }

            if (*text != '\0')
            {
                text++;
            }

            continue;
        }

        if (((unsigned char)*text & 0xC0) != 0x80)
        {
            length++;
        }

        text++;
    }

    return length;
}

/* Purely cosmetic progress bar (22 characters wide). */
static void uiBuildBar(char *buffer, double percentage)
{
    int filled;
    int i;

    filled = (int)(percentage / 100.0 * 22.0 + 0.5);

    if (filled < 0)
    {
        filled = 0;
    }

    if (filled > 22)
    {
        filled = 22;
    }

    for (i = 0; i < 22; i++)
    {
        if (i < filled)
        {
            memcpy(&buffer[i * 3], "█", 3);
        }
        else
        {
            memcpy(&buffer[i * 3], "░", 3);
        }
    }

    buffer[66] = '\0';
}

/* Color matching the existing NORMAL / MODERATE / HIGH logic. */
static const char *uiStatusColor(double percentage)
{
    if (percentage < 70.0)
    {
        return COLOR_GREEN;
    }

    if (percentage <= 85.0)
    {
        return COLOR_YELLOW;
    }

    return COLOR_RED;
}

static void uiBoxTop(const char *title)
{
    int inner = UI_WIDTH - 2;
    int used = (int)strlen(title) + 2;
    int left = (inner - used) / 2;
    int right = inner - used - left;
    int i;

    printf("%s┌", uiC(COLOR_CYAN));

    for (i = 0; i < left; i++)
    {
        printf("─");
    }

    printf(" %s%s%s%s ", uiC(COLOR_TITLE), title, uiC(COLOR_RESET), uiC(COLOR_CYAN));

    for (i = 0; i < right; i++)
    {
        printf("─");
    }

    printf("┐%s\n", uiC(COLOR_RESET));
}

static void uiBoxBottom(void)
{
    int i;

    printf("%s└", uiC(COLOR_CYAN));

    for (i = 0; i < UI_WIDTH - 2; i++)
    {
        printf("─");
    }

    printf("┘%s\n", uiC(COLOR_RESET));
}

static void uiBoxSection(const char *title)
{
    int inner = UI_WIDTH - 2;
    int used = (int)strlen(title) + 4;
    int right;
    int i;

    printf("%s├── %s%s%s%s ", uiC(COLOR_CYAN), uiC(COLOR_ACCENT), title, uiC(COLOR_RESET), uiC(COLOR_CYAN));

    right = inner - used;

    for (i = 0; i < right; i++)
    {
        printf("─");
    }

    printf("┤%s\n", uiC(COLOR_RESET));
}

/* One box row: "│  text ... │". Long text drops the right border. */
static void uiBoxLine(const char *text)
{
    int right = (UI_WIDTH - 2) - (2 + uiVisibleLength(text));
    int i;

    printf("%s│%s  %s", uiC(COLOR_CYAN), uiC(COLOR_RESET), text);

    if (right >= 1)
    {
        for (i = 0; i < right; i++)
        {
            printf(" ");
        }

        printf("%s│%s\n", uiC(COLOR_CYAN), uiC(COLOR_RESET));
    }
    else
    {
        printf("%s\n", uiC(COLOR_RESET));
    }
}

static void uiBoxCenterLine(const char *text)
{
    int inner = UI_WIDTH - 2;
    int visible = uiVisibleLength(text);
    int left = (inner - visible) / 2;
    int right;
    int i;

    if (left < 0)
    {
        left = 0;
    }

    printf("%s│%s", uiC(COLOR_CYAN), uiC(COLOR_RESET));

    for (i = 0; i < left; i++)
    {
        printf(" ");
    }

    printf("%s", text);

    right = inner - left - visible;

    for (i = 0; i < right; i++)
    {
        printf(" ");
    }

    printf("%s│%s\n", uiC(COLOR_CYAN), uiC(COLOR_RESET));
}

/* "Label : value" row with fixed label column. */
static void uiFieldLine(const char *label, const char *value)
{
    char line[512];

    snprintf(line, sizeof(line),
             "%s%-20s%s : %s%s%s",
             uiC(COLOR_GRAY),
             label,
             uiC(COLOR_RESET),
             uiC(COLOR_VALUE),
             value,
             uiC(COLOR_RESET));

    uiBoxLine(line);
}

/* Cosmetic "Label : ████░░  42.00%" row (CPU monitor). */
static void uiUsageLine(const char *label, double percentage)
{
    char bar[67];

    uiBuildBar(bar, percentage);

    char value[128];

    snprintf(value, sizeof(value),
             "%s%s%s %s%6.2f%%%s",
             uiC(uiStatusColor(percentage)),
             bar,
             uiC(COLOR_RESET),
             uiC(COLOR_BOLD),
             percentage,
             uiC(COLOR_RESET));

    uiFieldLine(label, value);
}

/* Cosmetic bar-only row (under memory and disk percentages). */
static void uiBarRow(double percentage)
{
    char bar[67];
    char line[128];

    uiBuildBar(bar, percentage);

    snprintf(line, sizeof(line),
             "%s%s%s  %s%6.2f%%%s",
             uiC(uiStatusColor(percentage)),
             bar,
             uiC(COLOR_RESET),
             uiC(COLOR_GRAY),
             percentage,
             uiC(COLOR_RESET));

    uiBoxLine(line);
}

/* Colored menu row: "[N] Text". */
static void uiMenuItem(const char *number, const char *text, const char *numberColor)
{
    char line[256];

    snprintf(line, sizeof(line),
             "%s[%s]%s %s%s%s",
             uiC(numberColor),
             number,
             uiC(COLOR_RESET),
             uiC(COLOR_WHITE),
             text,
             uiC(COLOR_RESET));

    uiBoxLine(line);
}

/* Colored NORMAL / MODERATE / HIGH status row. */
static void uiStatusLine(const char *label, const char *statusText, double percentage)
{
    char line[128];

    snprintf(line, sizeof(line),
             "%s%s%s",
             uiC(uiStatusColor(percentage)),
             statusText,
             uiC(COLOR_RESET));

    uiFieldLine(label, line);
}

/* Red error line, presentation only. */
static void uiError(const char *message)
{
    printf("\n%s[ ERROR ]%s %s%s%s\n",
           uiC(COLOR_RED),
           uiC(COLOR_RESET),
           uiC(COLOR_BOLD),
           message,
           uiC(COLOR_RESET));
}

/* 1. System Information */
void systemInformation()
{
    struct utsname info;

    char cpuModel[256] = "Unknown";
    char cpuFrequency[100] = "Unknown";

    long cpuCores;
    char *username;
    char *homeDirectory;

    /* Get basic system information */
    if (uname(&info) != 0)
    {
        printf("Unable to get system information.\n");
        return;
    }

    /* Get CPU information */
    FILE *file = fopen("/proc/cpuinfo", "r");

    if (file != NULL)
    {
        char line[256];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            if (strncmp(line, "model name", 10) == 0)
            {
                char *colon = strchr(line, ':');

                if (colon != NULL)
                {
                    strcpy(cpuModel, colon + 2);
                    cpuModel[strcspn(cpuModel, "\n")] = '\0';
                }
            }

            if (strncmp(line, "cpu MHz", 7) == 0)
            {
                char *colon = strchr(line, ':');

                if (colon != NULL)
                {
                    double frequency;

                    if (sscanf(colon + 1, "%lf", &frequency) == 1)
                    {
                        snprintf(cpuFrequency,
                                 sizeof(cpuFrequency),
                                 "%.2f MHz",
                                 frequency);
                    }
                }

                break;
            }
        }

        fclose(file);
    }

    /* Get number of CPU cores */
    cpuCores = sysconf(_SC_NPROCESSORS_ONLN);

    /* Get current username */
    username = getlogin();

    if (username == NULL)
    {
        username = getenv("USER");
    }

    /* Get home directory */
    homeDirectory = getenv("HOME");

    printf("\n");
    uiBoxTop("SYSTEM INFORMATION");

    uiBoxSection("SYSTEM DETAILS");
    uiBoxLine("");
    uiFieldLine("Operating System", info.sysname);
    uiFieldLine("Kernel Version", info.release);
    uiFieldLine("Architecture", info.machine);
    uiFieldLine("Hostname", info.nodename);
    uiBoxLine("");

    uiBoxSection("CPU INFORMATION");
    uiBoxLine("");
    uiFieldLine("CPU Model", cpuModel);

    if (cpuCores > 0)
    {
        char coresText[32];

        snprintf(coresText, sizeof(coresText), "%ld", cpuCores);
        uiFieldLine("CPU Cores", coresText);
    }
    else
    {
        uiFieldLine("CPU Cores", "Unknown");
    }

    uiFieldLine("CPU Frequency", cpuFrequency);
    uiBoxLine("");

    uiBoxSection("USER INFORMATION");
    uiBoxLine("");

    if (username != NULL)
    {
        uiFieldLine("Current User", username);
    }
    else
    {
        uiFieldLine("Current User", "Unknown");
    }

    if (homeDirectory != NULL)
    {
        uiFieldLine("Home Directory", homeDirectory);
    }
    else
    {
        uiFieldLine("Home Directory", "Unknown");
    }

    uiBoxLine("");
    uiBoxBottom();
}

int endKeyPressed()
{
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0)
    {
        char ch = getchar();

        if (ch == 27)
        {
            char ch2 = getchar();

            if (ch2 == '[')
            {
                char ch3 = getchar();

                if (ch3 == 'F')
                    return 1;

                if (ch3 == '4')
                {
                    char ch4 = getchar();

                    if (ch4 == '~')
                        return 1;
                }
            }
        }
    }

    return 0;
}

/* 2. CPU Usage */
void cpuUsage() {
    FILE *file;
    char line[256];

    long user1, nice1, system1, idle1;
    long iowait1, irq1, softirq1, steal1;

    long user2, nice2, system2, idle2;
    long iowait2, irq2, softirq2, steal2;

    /* Get first CPU reading */
    file = fopen("/proc/stat", "r");

    if (file == NULL) {
        printf("Unable to read CPU information.\n");
        return;
    }

    fgets(line, sizeof(line), file);

    sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user1, &nice1, &system1, &idle1,
           &iowait1, &irq1, &softirq1, &steal1);

    fclose(file);

    /* Wait one second */
    sleep(1);

    /* Get second CPU reading */
    file = fopen("/proc/stat", "r");

    if (file == NULL) {
        printf("Unable to read CPU information.\n");
        return;
    }

    fgets(line, sizeof(line), file);

    sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user2, &nice2, &system2, &idle2,
           &iowait2, &irq2, &softirq2, &steal2);

    fclose(file);

    long total1 = user1 + nice1 + system1 + idle1 +
                  iowait1 + irq1 + softirq1 + steal1;

    long total2 = user2 + nice2 + system2 + idle2 +
                  iowait2 + irq2 + softirq2 + steal2;

    long idleTotal1 = idle1 + iowait1;
    long idleTotal2 = idle2 + iowait2;

    long totalDifference = total2 - total1;
    long idleDifference = idleTotal2 - idleTotal1;

    double currentUsage = 0.0;

    if (totalDifference > 0) {
        currentUsage =
            100.0 * (totalDifference - idleDifference)
            / totalDifference;
    }

    printf("\n");
    uiBoxTop("CPU MONITOR");
    uiBoxLine("");
    uiUsageLine("CPU Usage", currentUsage);
    uiBoxLine("");
    uiBoxBottom();

    int choice;

printf("\n%s0.%s %sStart CPU Monitoring%s\n",
       uiC(COLOR_CYAN),
       uiC(COLOR_RESET),
       uiC(COLOR_WHITE),
       uiC(COLOR_RESET));
printf("%s1.%s %sReturn to Main Menu%s\n",
       uiC(COLOR_CYAN),
       uiC(COLOR_RESET),
       uiC(COLOR_WHITE),
       uiC(COLOR_RESET));
printf("%s2.%s %sExit%s\n",
       uiC(COLOR_RED),
       uiC(COLOR_RESET),
       uiC(COLOR_WHITE),
       uiC(COLOR_RESET));

printf("\n%sEnter your choice:%s ",
       uiC(COLOR_BOLD),
       uiC(COLOR_RESET));
scanf("%d", &choice);

if (choice == 1)
{
    return;
}

if (choice == 2)
{
    printf("\nExiting Linux System Monitor...\n");
    printf("Thank you!\n\n");
    exit(0);
}

if (choice != 0)
{
    uiError("Invalid choice. Returning to Main Menu...");
    return;
}
struct termios oldTerminal;
struct termios newTerminal;

tcgetattr(STDIN_FILENO, &oldTerminal);

newTerminal = oldTerminal;
newTerminal.c_lflag &= ~(ICANON | ECHO);

tcsetattr(STDIN_FILENO, TCSANOW, &newTerminal);

    printf("\n");
    uiBoxTop("REAL-TIME CPU MONITORING");
    uiBoxLine("");
    {
        char hint[128];

        snprintf(hint, sizeof(hint),
                 "%sPress %sEnd%s%s key to stop monitoring%s",
                 uiC(COLOR_GRAY),
                 uiC(COLOR_BOLD),
                 uiC(COLOR_RESET),
                 uiC(COLOR_GRAY),
                 uiC(COLOR_RESET));

        uiBoxLine(hint);
        uiBoxBottom();
    }
    printf("\n");

    /*
     * Continuous monitoring.
     * Currently Ctrl+C can be used to stop it.
     * We will add the actual END-key handling
     * after testing the CPU monitoring.
     */

    while (1) {
        if (endKeyPressed()) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminal);
        printf("\n\n%s[ INFO ]%s CPU monitoring stopped.%s\n",
               uiC(COLOR_GREEN),
               uiC(COLOR_RESET),
               uiC(COLOR_RESET));
        printf("%sReturning to Main Menu...%s\n",
               uiC(COLOR_GRAY),
               uiC(COLOR_RESET));
        sleep(1);
        return;
    }
        file = fopen("/proc/stat", "r");

        if (file == NULL) {
            printf("Unable to read CPU information.\n");
            return;
        }

        fgets(line, sizeof(line), file);

        sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
               &user1, &nice1, &system1, &idle1,
               &iowait1, &irq1, &softirq1, &steal1);

        fclose(file);

        sleep(1);

        file = fopen("/proc/stat", "r");

        if (file == NULL) {
            printf("Unable to read CPU information.\n");
            return;
        }

        fgets(line, sizeof(line), file);

        sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
               &user2, &nice2, &system2, &idle2,
               &iowait2, &irq2, &softirq2, &steal2);

        fclose(file);

        total1 = user1 + nice1 + system1 + idle1 +
                 iowait1 + irq1 + softirq1 + steal1;

        total2 = user2 + nice2 + system2 + idle2 +
                 iowait2 + irq2 + softirq2 + steal2;

        idleTotal1 = idle1 + iowait1;
        idleTotal2 = idle2 + iowait2;

        totalDifference = total2 - total1;
        idleDifference = idleTotal2 - idleTotal1;

        if (totalDifference > 0) {

            double usage =
                100.0 * (totalDifference - idleDifference)
                / totalDifference;

            {
                char bar[67];

                uiBuildBar(bar, usage);

                printf("%s%s%s %s%6.2f%%%s\n",
                       uiC(uiStatusColor(usage)),
                       bar,
                       uiC(COLOR_RESET),
                       uiC(COLOR_BOLD),
                       usage,
                       uiC(COLOR_RESET));
            }

            fflush(stdout);
        }
    }
}

/* 3. Memory Usage */
void memoryUsage()
{
    FILE *file;
    char line[256];

    long totalMemory = 0;
    long availableMemory = 0;
    long cachedMemory = 0;
    long buffers = 0;
    long sharedMemory = 0;
    long activeMemory = 0;
    long inactiveMemory = 0;

    long totalSwap = 0;
    long freeSwap = 0;

    file = fopen("/proc/meminfo", "r");

    if (file == NULL)
    {
        printf("Unable to read memory information.\n");
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strncmp(line, "MemTotal:", 9) == 0)
        {
            sscanf(line, "MemTotal: %ld kB", &totalMemory);
        }
        else if (strncmp(line, "MemAvailable:", 13) == 0)
        {
            sscanf(line, "MemAvailable: %ld kB", &availableMemory);
        }
        else if (strncmp(line, "Cached:", 7) == 0)
        {
            sscanf(line, "Cached: %ld kB", &cachedMemory);
        }
        else if (strncmp(line, "Buffers:", 8) == 0)
        {
            sscanf(line, "Buffers: %ld kB", &buffers);
        }
        else if (strncmp(line, "Shmem:", 6) == 0)
        {
            sscanf(line, "Shmem: %ld kB", &sharedMemory);
        }
        else if (strncmp(line, "Active:", 7) == 0)
        {
            sscanf(line, "Active: %ld kB", &activeMemory);
        }
        else if (strncmp(line, "Inactive:", 9) == 0)
        {
            sscanf(line, "Inactive: %ld kB", &inactiveMemory);
        }
        else if (strncmp(line, "SwapTotal:", 10) == 0)
        {
            sscanf(line, "SwapTotal: %ld kB", &totalSwap);
        }
        else if (strncmp(line, "SwapFree:", 9) == 0)
        {
            sscanf(line, "SwapFree: %ld kB", &freeSwap);
        }
    }

    fclose(file);

    if (totalMemory > 0)
    {
        long usedMemory = totalMemory - availableMemory;

        double memoryPercentage =
            ((double)usedMemory / totalMemory) * 100.0;

        long usedSwap = totalSwap - freeSwap;

        double swapPercentage = 0.0;

        if (totalSwap > 0)
        {
            swapPercentage =
                ((double)usedSwap / totalSwap) * 100.0;
        }

        /* Presentation-only: pre-formatted field values. */
        char ramTotalText[64];
        char ramUsedText[64];
        char ramAvailableText[64];
        char memoryPercentageText[64];
        char cachedText[64];
        char buffersText[64];
        char sharedText[64];
        char activeText[64];
        char inactiveText[64];
        char swapTotalText[64];
        char swapUsedText[64];
        char swapFreeText[64];
        char swapPercentageText[64];
        const char *memoryStatusText;

        snprintf(ramTotalText, sizeof(ramTotalText),
                 "%.2f GB",
                 totalMemory / (1024.0 * 1024.0));

        snprintf(ramUsedText, sizeof(ramUsedText),
                 "%.2f GB",
                 usedMemory / (1024.0 * 1024.0));

        snprintf(ramAvailableText, sizeof(ramAvailableText),
                 "%.2f GB",
                 availableMemory / (1024.0 * 1024.0));

        snprintf(memoryPercentageText, sizeof(memoryPercentageText),
                 "%.2f%%",
                 memoryPercentage);

        snprintf(cachedText, sizeof(cachedText),
                 "%.2f GB",
                 cachedMemory / (1024.0 * 1024.0));

        snprintf(buffersText, sizeof(buffersText),
                 "%.2f GB",
                 buffers / (1024.0 * 1024.0));

        snprintf(sharedText, sizeof(sharedText),
                 "%.2f GB",
                 sharedMemory / (1024.0 * 1024.0));

        snprintf(activeText, sizeof(activeText),
                 "%.2f GB",
                 activeMemory / (1024.0 * 1024.0));

        snprintf(inactiveText, sizeof(inactiveText),
                 "%.2f GB",
                 inactiveMemory / (1024.0 * 1024.0));

        snprintf(swapTotalText, sizeof(swapTotalText),
                 "%.2f GB",
                 totalSwap / (1024.0 * 1024.0));

        snprintf(swapUsedText, sizeof(swapUsedText),
                 "%.2f GB",
                 usedSwap / (1024.0 * 1024.0));

        snprintf(swapFreeText, sizeof(swapFreeText),
                 "%.2f GB",
                 freeSwap / (1024.0 * 1024.0));

        snprintf(swapPercentageText, sizeof(swapPercentageText),
                 "%.2f%%",
                 swapPercentage);

        if (memoryPercentage < 70.0)
        {
            memoryStatusText = "NORMAL";
        }
        else if (memoryPercentage <= 85.0)
        {
            memoryStatusText = "MODERATE";
        }
        else
        {
            memoryStatusText = "HIGH";
        }

        printf("\n");
        uiBoxTop("MEMORY INFORMATION");

        uiBoxSection("RAM USAGE");
        uiBoxLine("");
        uiFieldLine("Total Memory", ramTotalText);
        uiFieldLine("Used Memory", ramUsedText);
        uiFieldLine("Available Memory", ramAvailableText);
        uiFieldLine("Memory Usage", memoryPercentageText);
        uiBarRow(memoryPercentage);
        uiBoxLine("");

        uiBoxSection("MEMORY DETAILS");
        uiBoxLine("");
        uiFieldLine("Cached Memory", cachedText);
        uiFieldLine("Buffers", buffersText);
        uiFieldLine("Shared Memory", sharedText);
        uiFieldLine("Active Memory", activeText);
        uiFieldLine("Inactive Memory", inactiveText);
        uiBoxLine("");

        uiBoxSection("SWAP MEMORY");
        uiBoxLine("");
        uiFieldLine("Total Swap", swapTotalText);
        uiFieldLine("Used Swap", swapUsedText);
        uiFieldLine("Free Swap", swapFreeText);
        uiFieldLine("Swap Usage", swapPercentageText);
        uiBarRow(swapPercentage);
        uiBoxLine("");

        uiBoxSection("MEMORY STATUS");
        uiBoxLine("");
        uiStatusLine("Status", memoryStatusText, memoryPercentage);
        uiBoxLine("");
        uiBoxBottom();
    }
}

/* 4. Disk Usage */
void diskUsage()
{
    struct statvfs disk;

    if (statvfs("/", &disk) != 0)
    {
        printf("Unable to read disk information.\n");
        return;
    }

    /* Disk space calculations */
    unsigned long long total =
        (unsigned long long)disk.f_blocks *
        disk.f_frsize;

    unsigned long long freeSpace =
        (unsigned long long)disk.f_bfree *
        disk.f_frsize;

    unsigned long long used =
        total - freeSpace;

    double diskPercentage =
        ((double)used / total) * 100.0;

    /* Inode calculations */
    unsigned long long totalInodes =
        (unsigned long long)disk.f_files;

    unsigned long long freeInodes =
        (unsigned long long)disk.f_ffree;

    unsigned long long usedInodes =
        totalInodes - freeInodes;

    double inodePercentage = 0.0;

    if (totalInodes > 0)
    {
        inodePercentage =
            ((double)usedInodes / totalInodes) * 100.0;
    }

    /* Presentation-only: pre-formatted field values. */
    char totalDiskText[64];
    char usedDiskText[64];
    char freeDiskText[64];
    char diskPercentageText[64];
    char totalInodesText[64];
    char usedInodesText[64];
    char freeInodesText[64];
    char inodePercentageText[64];
    const char *diskStatusText;

    snprintf(totalDiskText, sizeof(totalDiskText),
             "%.2f GB",
             total / (1024.0 * 1024.0 * 1024.0));

    snprintf(usedDiskText, sizeof(usedDiskText),
             "%.2f GB",
             used / (1024.0 * 1024.0 * 1024.0));

    snprintf(freeDiskText, sizeof(freeDiskText),
             "%.2f GB",
             freeSpace / (1024.0 * 1024.0 * 1024.0));

    snprintf(diskPercentageText, sizeof(diskPercentageText),
             "%.2f%%",
             diskPercentage);

    snprintf(totalInodesText, sizeof(totalInodesText),
             "%llu",
             totalInodes);

    snprintf(usedInodesText, sizeof(usedInodesText),
             "%llu",
             usedInodes);

    snprintf(freeInodesText, sizeof(freeInodesText),
             "%llu",
             freeInodes);

    snprintf(inodePercentageText, sizeof(inodePercentageText),
             "%.2f%%",
             inodePercentage);

    if (diskPercentage < 70.0)
    {
        diskStatusText = "NORMAL";
    }
    else if (diskPercentage <= 85.0)
    {
        diskStatusText = "MODERATE";
    }
    else
    {
        diskStatusText = "HIGH";
    }

    printf("\n");
    uiBoxTop("DISK INFORMATION");

    uiBoxSection("DISK SPACE");
    uiBoxLine("");
    uiFieldLine("Total Disk Space", totalDiskText);
    uiFieldLine("Used Disk Space", usedDiskText);
    uiFieldLine("Free Disk Space", freeDiskText);
    uiFieldLine("Disk Usage", diskPercentageText);
    uiBarRow(diskPercentage);
    uiBoxLine("");

    uiBoxSection("DISK STATUS");
    uiBoxLine("");
    uiStatusLine("Status", diskStatusText, diskPercentage);
    uiBoxLine("");

    uiBoxSection("INODE INFORMATION");
    uiBoxLine("");
    uiFieldLine("Total Inodes", totalInodesText);
    uiFieldLine("Used Inodes", usedInodesText);
    uiFieldLine("Free Inodes", freeInodesText);
    uiFieldLine("Inode Usage", inodePercentageText);
    uiBoxLine("");
    uiBoxBottom();
}

typedef struct
{
    int pid;
    int ppid;
    char state;
    char name[256];
} ProcessInfo;


void printProcessTree(ProcessInfo processes[], int count,
                      int parentPid, int level)
{
    for (int i = 0; i < count; i++)
    {
        if (processes[i].ppid == parentPid)
        {
            for (int j = 0; j < level; j++)
            {
                printf("    ");
            }

            if (level > 0)
            {
                printf("|-- ");
            }

            printf("%s (PID: %d)\n",
                   processes[i].name,
                   processes[i].pid);

            printProcessTree(processes,
                             count,
                             processes[i].pid,
                             level + 1);
        }
    }
}

/* 5. Running Processes */
void processMonitoring()
{
    DIR *directory;
    struct dirent *entry;

    ProcessInfo processes[4096];
    int processCount = 0;

    int runningCount = 0;
    int sleepingCount = 0;
    int stoppedCount = 0;
    int otherCount = 0;

    directory = opendir("/proc");

    if (directory == NULL)
    {
        printf("Unable to access /proc.\n");
        return;
    }

    while ((entry = readdir(directory)) != NULL)
    {
        if (entry->d_type != DT_DIR)
        {
            continue;
        }

        int isNumber = 1;

        for (int i = 0; entry->d_name[i] != '\0'; i++)
        {
            if (entry->d_name[i] < '0' ||
                entry->d_name[i] > '9')
            {
                isNumber = 0;
                break;
            }
        }

        if (!isNumber)
        {
            continue;
        }

        if (processCount >= 4096)
        {
            break;
        }

        int pid = atoi(entry->d_name);

        char commPath[512];
        char statPath[512];

        snprintf(commPath,
                 sizeof(commPath),
                 "/proc/%s/comm",
                 entry->d_name);

        snprintf(statPath,
                 sizeof(statPath),
                 "/proc/%s/stat",
                 entry->d_name);

        /* Get process name */
        FILE *commFile = fopen(commPath, "r");

        if (commFile == NULL)
        {
            continue;
        }

        if (fgets(processes[processCount].name,
                  sizeof(processes[processCount].name),
                  commFile) == NULL)
        {
            fclose(commFile);
            continue;
        }

        fclose(commFile);

        processes[processCount].name[
            strcspn(processes[processCount].name, "\n")
        ] = '\0';

        /* Get process state and PPID */
        FILE *statFile = fopen(statPath, "r");

        if (statFile == NULL)
        {
            continue;
        }

        int readPid;
        char processState;
        int parentPid;

        if (fscanf(statFile,
                   "%d %*[^)] ) %c %d",
                   &readPid,
                   &processState,
                   &parentPid) != 3)
        {
            fclose(statFile);
            continue;
        }

        fclose(statFile);

        processes[processCount].pid = pid;
        processes[processCount].ppid = parentPid;
        processes[processCount].state = processState;

        /* Count process states */
        if (processState == 'R')
        {
            runningCount++;
        }
        else if (processState == 'S' ||
                 processState == 'D')
        {
            sleepingCount++;
        }
        else if (processState == 'T')
        {
            stoppedCount++;
        }
        else
        {
            otherCount++;
        }

        processCount++;
    }

    closedir(directory);

    /* Process information */
    printf("\n");
    uiBoxTop("PROCESS INFORMATION");

    uiBoxSection("PROCESS LIST");
    uiBoxLine("");
    printf("%s│%s  %s%-8s %-8s %-8s%s %s%-24s %s%s\n",
           uiC(COLOR_CYAN),
           uiC(COLOR_RESET),
           uiC(COLOR_GRAY),
           "PID",
           "PPID",
           "STATE",
           uiC(COLOR_RESET),
           uiC(COLOR_GRAY),
           "PROCESS",
           uiC(COLOR_RESET),
           uiC(COLOR_CYAN));
    printf("%s│%s  %s",
           uiC(COLOR_CYAN),
           uiC(COLOR_RESET),
           uiC(COLOR_GRAY));

    for (int i = 0; i < 54; i++)
    {
        printf("─");
    }

    printf("%s\n", uiC(COLOR_RESET));

    for (int i = 0; i < processCount; i++)
    {
        const char *stateColor = COLOR_WHITE;

        if (processes[i].state == 'R')
        {
            stateColor = COLOR_GREEN;
        }
        else if (processes[i].state == 'S' ||
                 processes[i].state == 'D')
        {
            stateColor = COLOR_CYAN;
        }
        else if (processes[i].state == 'T')
        {
            stateColor = COLOR_YELLOW;
        }
        else
        {
            stateColor = COLOR_GRAY;
        }

        printf("%s│%s  %-8d %-8d %s%c%s       %s%s%s\n",
               uiC(COLOR_CYAN),
               uiC(COLOR_RESET),
               processes[i].pid,
               processes[i].ppid,
               uiC(stateColor),
               processes[i].state,
               uiC(COLOR_RESET),
               uiC(COLOR_WHITE),
               processes[i].name,
               uiC(COLOR_RESET));
    }

    uiBoxLine("");

    /* Process summary */
    uiBoxSection("PROCESS SUMMARY");
    uiBoxLine("");
    {
        char valueText[64];

        snprintf(valueText, sizeof(valueText), "%d", processCount);
        uiFieldLine("Total Processes", valueText);

        snprintf(valueText, sizeof(valueText), "%d", runningCount);
        uiFieldLine("Running Processes", valueText);

        snprintf(valueText, sizeof(valueText), "%d", sleepingCount);
        uiFieldLine("Sleeping Processes", valueText);

        snprintf(valueText, sizeof(valueText), "%d", stoppedCount);
        uiFieldLine("Stopped Processes", valueText);

        snprintf(valueText, sizeof(valueText), "%d", otherCount);
        uiFieldLine("Other Processes", valueText);
    }
    uiBoxLine("");

    /* Process tree */
    uiBoxSection("PROCESS TREE");
    uiBoxLine("");

    /*
     * Linux normally starts the process tree
     * from PID 1.
     */
    printProcessTree(processes,
                     processCount,
                     0,
                     0);

    uiBoxLine("");
    uiBoxBottom();
}

/* 6. System Uptime */
void systemUptime()
{
    FILE *file;
    double uptimeSeconds;
    double cpuUserTime;
    double cpuNiceTime;
    double cpuSystemTime;
    double cpuIdleTime;
    double cpuIowaitTime;
    double cpuIrqTime;
    double cpuSoftirqTime;
    double cpuStealTime;

    /* Read system uptime */
    file = fopen("/proc/uptime", "r");

    if (file == NULL)
    {
        printf("Unable to read system uptime.\n");
        return;
    }

    if (fscanf(file, "%lf", &uptimeSeconds) != 1)
    {
        fclose(file);
        printf("Unable to read system uptime.\n");
        return;
    }

    fclose(file);

    /* Convert uptime into days, hours, minutes and seconds */
    long totalSeconds = (long)uptimeSeconds;

    long days = totalSeconds / 86400;
    long remainingSeconds = totalSeconds % 86400;

    long hours = remainingSeconds / 3600;
    remainingSeconds %= 3600;

    long minutes = remainingSeconds / 60;
    long seconds = remainingSeconds % 60;

    /* Read CPU time since boot */
    file = fopen("/proc/stat", "r");

    double totalCPUTime = 0.0;

    if (file != NULL)
    {
        char line[256];

        if (fgets(line, sizeof(line), file) != NULL)
        {
            sscanf(line,
                   "cpu %lf %lf %lf %lf %lf %lf %lf %lf",
                   &cpuUserTime,
                   &cpuNiceTime,
                   &cpuSystemTime,
                   &cpuIdleTime,
                   &cpuIowaitTime,
                   &cpuIrqTime,
                   &cpuSoftirqTime,
                   &cpuStealTime);

            totalCPUTime =
                cpuUserTime +
                cpuNiceTime +
                cpuSystemTime +
                cpuIdleTime +
                cpuIowaitTime +
                cpuIrqTime +
                cpuSoftirqTime +
                cpuStealTime;

            /*
             * Linux CPU times are normally measured
             * in clock ticks. Convert ticks to seconds.
             */
            long clockTicks = sysconf(_SC_CLK_TCK);

            if (clockTicks > 0)
            {
                totalCPUTime =
                    totalCPUTime / clockTicks;
            }
        }

        fclose(file);
    }

    /* Get number of CPU cores */
    long cpuCores = sysconf(_SC_NPROCESSORS_ONLN);

    /* Presentation-only: pre-formatted field values. */
    char uptimeText[128];
    char totalHoursText[64];
    char totalMinutesText[64];
    char totalSecondsText[64];
    char cpuCoresText[64];
    char cpuTimeText[64];

    snprintf(uptimeText, sizeof(uptimeText),
             "%ld Days, %ld Hours, %ld Minutes, %ld Seconds",
             days,
             hours,
             minutes,
             seconds);

    snprintf(totalHoursText, sizeof(totalHoursText),
             "%.2f Hours",
             uptimeSeconds / 3600.0);

    snprintf(totalMinutesText, sizeof(totalMinutesText),
             "%.2f Minutes",
             uptimeSeconds / 60.0);

    snprintf(totalSecondsText, sizeof(totalSecondsText),
             "%.0f Seconds",
             uptimeSeconds);

    snprintf(cpuTimeText, sizeof(cpuTimeText),
             "%.2f Seconds",
             totalCPUTime);

    printf("\n");
    uiBoxTop("SYSTEM UPTIME");

    uiBoxSection("UPTIME");
    uiBoxLine("");
    uiFieldLine("Uptime", uptimeText);
    uiFieldLine("Total Uptime Hours", totalHoursText);
    uiFieldLine("Total Uptime Minutes", totalMinutesText);
    uiFieldLine("Total Uptime Seconds", totalSecondsText);
    uiBoxLine("");

    uiBoxSection("SYSTEM STATUS");
    uiBoxLine("");
    uiFieldLine("System Status", "RUNNING");
    uiBoxLine("");

    uiBoxSection("CPU INFORMATION");
    uiBoxLine("");

    if (cpuCores > 0)
    {
        snprintf(cpuCoresText, sizeof(cpuCoresText), "%ld", cpuCores);
        uiFieldLine("CPU Cores", cpuCoresText);
    }

    uiFieldLine("CPU Time Since Boot", cpuTimeText);
    uiBoxLine("");
    uiBoxBottom();
}

/* After Feature Menu */
void afterFeature()
{
    int choice;

    printf("\n");
    uiBoxTop("OPTIONS");
    uiMenuItem("1", "Return to Main Menu", COLOR_CYAN);
    uiMenuItem("2", "Exit", COLOR_RED);
    uiBoxBottom();

    printf("\n%sEnter your choice:%s ",
           uiC(COLOR_BOLD),
           uiC(COLOR_RESET));

    if (scanf("%d", &choice) != 1)
    {
        uiError("Invalid input. Returning to Main Menu...");

        while (getchar() != '\n')
        {
            /* Clear invalid input */
        }

        sleep(1);
        return;
    }

    if (choice == 2)
    {
        printf("\n%sExiting Linux System Monitor...%s\n",
               uiC(COLOR_YELLOW),
               uiC(COLOR_RESET));
        printf("%sThank you!%s\n\n",
               uiC(COLOR_BOLD),
               uiC(COLOR_RESET));
        exit(0);
    }

    if (choice != 1)
    {
        uiError("Invalid choice. Returning to Main Menu...");
    }
}

/* Clear Screen */
void clearScreen()
{
    printf("\033[2J\033[H");
}

/* Main Function */
int main()
{
    int choice;

    while (1)
    {
        clearScreen();
        printf("\n");
        uiBoxTop("FORGEOS");
        uiBoxCenterLine("Linux System Monitor");
        uiBoxCenterLine("");
        uiBoxLine("");
        uiMenuItem("1", "System Information", COLOR_CYAN);
        uiMenuItem("2", "CPU Usage", COLOR_CYAN);
        uiMenuItem("3", "Memory Usage", COLOR_CYAN);
        uiMenuItem("4", "Disk Usage", COLOR_CYAN);
        uiMenuItem("5", "Running Processes", COLOR_CYAN);
        uiMenuItem("6", "System Uptime", COLOR_CYAN);
        uiMenuItem("7", "Exit", COLOR_RED);
        uiBoxLine("");
        uiBoxBottom();

        printf("\n%sEnter your choice:%s ",
               uiC(COLOR_BOLD),
               uiC(COLOR_RESET));

        if (scanf("%d", &choice) != 1)
        {
            uiError("Invalid input. Please enter a number.");

            while (getchar() != '\n')
            {
                /* Clear invalid input */
            }

            sleep(1);
            continue;
        }

        switch (choice)
        {
            case 1:
                systemInformation();
                afterFeature();
                break;

            case 2:
                cpuUsage();
                break;

            case 3:
                memoryUsage();
                afterFeature();
                break;

            case 4:
                diskUsage();
                afterFeature();
                break;

            case 5:
                processMonitoring();
                afterFeature();
                break;

            case 6:
                systemUptime();
                afterFeature();
                break;

            case 7:
                printf("\n%sExiting Linux System Monitor...%s\n",
                       uiC(COLOR_YELLOW),
                       uiC(COLOR_RESET));
                printf("%sThank you!%s\n\n",
                       uiC(COLOR_BOLD),
                       uiC(COLOR_RESET));
                return 0;

            default:
                uiError("Invalid choice. Please select 1-7.");
                sleep(1);
        }
    }

    return 0;
}