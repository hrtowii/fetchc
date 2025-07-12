#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/sysctl.h>
// uptime
#include <time.h>
#include <unistd.h>
// package listing
#include <dirent.h>
// ram
#include <mach/mach.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
// color
#include "color.h"
//battery info
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

// static void printDictionaryEntry(const void *key, const void *value, void *context) {
//     if (CFGetTypeID(key) == CFStringGetTypeID() && CFGetTypeID(value) == CFNumberGetTypeID()) {
//         char buffer[256];
//         CFStringGetCString((CFStringRef)key, buffer, sizeof(buffer), kCFStringEncodingUTF8);
//         int intValue;
//         CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &intValue);
//         printf("%s: %d\n", buffer, intValue);
//     } else if (CFGetTypeID(key) == CFStringGetTypeID() && CFGetTypeID(value) == CFBooleanGetTypeID()) {
//         char buffer[256];
//         CFStringGetCString((CFStringRef)key, buffer, sizeof(buffer), kCFStringEncodingUTF8);
//         bool boolValue = CFBooleanGetValue((CFBooleanRef)value);
//         printf("%s: %s\n", buffer, boolValue ? "true" : "false");
//     } else if (CFGetTypeID(key) == CFStringGetTypeID() && CFGetTypeID(value) == CFStringGetTypeID()) {
//         char keyBuffer[256];
//         char valueBuffer[256];
//         CFStringGetCString((CFStringRef)key, keyBuffer, sizeof(keyBuffer), kCFStringEncodingUTF8);
//         CFStringGetCString((CFStringRef)value, valueBuffer, sizeof(valueBuffer), kCFStringEncodingUTF8);
//         printf("%s: %s\n", keyBuffer, valueBuffer);
//     }
// }

int get_cycle_count() {
    io_service_t powerSource = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPMPowerSource"));
    CFMutableDictionaryRef batteryProperties = NULL;
    IORegistryEntryCreateCFProperties(powerSource, &batteryProperties, NULL, 0);
    // printf("Battery Properties:\n");
    // CFDictionaryApplyFunction(batteryProperties, printDictionaryEntry, NULL);
    int cycleCount = -1;
    CFNumberRef cycleCountValue = (CFNumberRef)CFDictionaryGetValue(batteryProperties, CFSTR("CycleCount"));
    if (cycleCountValue != NULL) {
        CFNumberGetValue(cycleCountValue, kCFNumberIntType, &cycleCount);
    }
    if (batteryProperties != NULL) {
        CFRelease(batteryProperties);
    }
    IOObjectRelease(powerSource);
    return cycleCount;
}

int get_battery_percentage() {
    CFTypeRef powerSourceInfo = IOPSCopyPowerSourcesInfo();
    CFArrayRef powerSources = IOPSCopyPowerSourcesList(powerSourceInfo);
    CFDictionaryRef powerSource = NULL;

    if (CFArrayGetCount(powerSources) > 0) {
        powerSource = (CFDictionaryRef)CFArrayGetValueAtIndex(powerSources, 0);
        // Current Capacity
        CFNumberRef currentCapacity = (CFNumberRef)CFDictionaryGetValue(powerSource, CFSTR(kIOPSCurrentCapacityKey));
        if (currentCapacity) {
            int current;
            CFNumberGetValue(currentCapacity, kCFNumberIntType, &current);
            printf("Current Capacity: %d\n", current);
        }

        // Maximum Capacity
        CFNumberRef maxCapacity = (CFNumberRef)CFDictionaryGetValue(powerSource, CFSTR(kIOPSMaxCapacityKey));
        if (maxCapacity) {
            int max;
            CFNumberGetValue(maxCapacity, kCFNumberIntType, &max);
            printf("Maximum Capacity: %d\n", max);
        }
    }

    CFRelease(powerSourceInfo);
    CFRelease(powerSources);
    return -1;
}

struct dist {
	char *col1, *col2, *col3, *col4, *col5, *col6, *col7, *col8;
};

struct dist info = {
	.col1 = BWHITE "     ___   \n",
	.col2 = BWHITE " ___/   \\___ ",
	.col3 = BWHITE "/   '---'   \\",
	.col4 = BWHITE "'--_______--'",
	.col5 = BWHITE "     / \\     ",
	.col6 = BWHITE "    /   \\    ",
	.col7 = BWHITE "   /     \\   ",
	.col8 = BWHITE "",
};

time_t get_boot_time() {
    struct timeval tv;
    size_t tv_size = sizeof(struct timeval);
    int err = sysctlbyname("kern.boottime", &tv, &tv_size, NULL, 0);
    if (err != 0 || tv_size != sizeof(struct timeval)) {
        return 0;
    }
    return tv.tv_sec;
}

void print_uptime(time_t uptime) {
    time_t now = time(NULL);
    time_t boot_time = now - uptime;
    struct tm* boot_tm = gmtime(&boot_time);

    printf("%s uptime    %s", info.col3, WHITE);
    if (boot_tm->tm_year != 70) {
        printf("%d years, ", boot_tm->tm_year + 1900);
    }
    printf("%d days, %02d:%02d:%02d\n", boot_tm->tm_yday, boot_tm->tm_hour, boot_tm->tm_min, boot_tm->tm_sec);
}

char* get_version() {
    char our_version[256];
    size_t size = sizeof(our_version);
    if (sysctlbyname("kern.version", our_version, &size, NULL, 0) != 0) {
        return strdup("unknown");
    } else {
        char* colon = strchr(our_version, ':');
        if (colon != NULL) {
            *colon = '\0';
        }
        return strdup(our_version);
    }
}

char* get_macOS() {
    char osversion[32];
    size_t osversion_len = sizeof(osversion);
    int osversion_name[] = { CTL_KERN, KERN_OSRELEASE };

    if (sysctl(osversion_name, 2, osversion, &osversion_len, NULL, 0) == -1) {
        fprintf(stderr, "sysctl() failed\n");
        return NULL;
    }

    osversion[osversion_len] = '\0'; // Null-terminate the string

    uint32_t major, minor;
    if (sscanf(osversion, "%u.%u", &major, &minor) != 2) {
        fprintf(stderr, "sscanf() failed\n");
        return NULL;
    }

    int intversion = (int)major;
    if (major >= 20) {
        intversion -= 9;
    } else {
        intversion -= 4;
    }

    char* version_string = malloc(16 * sizeof(char));
    if (version_string == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }

    snprintf(version_string, 16, "%d.%u", intversion, minor);
    return version_string;
}

char* get_hostname() {
	static char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	return hostname;
}

int count_directories(const char *path) { // begged claude for this as well.......
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

int get_installed_packages_brew(void) {
	int count = 0;
	count += count_directories("/opt/homebrew/Cellar");
	count += count_directories("/opt/homebrew/Caskroom");
	return count;
}

double get_total_ram(void) { // asked claude for this... but stackoverflow has the same solution so whatever https://stackoverflow.com/questions/583736/determine-physical-mem-size-programmatically-on-osx
	uint64_t ram;
    size_t len = sizeof(ram);
    int mib[] = {CTL_HW, HW_MEMSIZE};
    if (sysctl(mib, 2, &ram, &len, NULL, 0) != 0) {
        perror("sysctl");
        return 1;
    }
    return (double)ram / (1024 * 1024);
}

// how to get used ram? https://github.com/alvarofe/osxinternals/blob/master/host_statistics.c
double get_used_ram(void) {
	kern_return_t				kr;
	mach_msg_type_number_t		count;
	vm_statistics_data_t		vm_stat;
	count = HOST_VM_INFO_COUNT;
	kr = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_stat, &count);
	uint64_t used_memory = 0;
	used_memory += (int64_t)vm_stat.active_count + (int64_t)vm_stat.inactive_count + (int64_t)vm_stat.wire_count;
    used_memory *= vm_page_size;
	return (double)used_memory / (1024 * 1024);
}

int main() {
    info.col1 = "" BYELLOW;
	info.col2 = BGREEN "        .:'     " BRED;
	info.col3 = BGREEN "    __ :'__     " BRED;
	info.col4 = BYELLOW " .'`__`-'__``.  " BRED;
	info.col5 = BRED ":__________.-'  " BRED;
	info.col6 = BRED ":_________:     " BRED;
	info.col7 = BMAGENTA " :_________`-;  " BRED;
	info.col8 = BBLUE "  `.__.-.__.'   " BRED;
	char* our_version = get_version();
	time_t our_uptime = get_boot_time();
	char* our_hostname = get_hostname();
	char* our_macOS_vers = get_macOS();
	printf("%s", 										info.col1);
	printf("%s host      %s%s\n", 						info.col2, WHITE, our_hostname);
	print_uptime(our_uptime);
	printf("%s macOS	   %s%s\n%s kernel    %s%s\n", 	info.col4, WHITE, our_macOS_vers, info.col5, WHITE, our_version);
	printf("%s installed %s%d\n", 						info.col6, WHITE, get_installed_packages_brew());
	printf("%s ram	   %s%3.0fM / %3.0fM\n", 		info.col7, WHITE, get_used_ram(), get_total_ram());
    printf("%s cycles    %s%d\n", 						info.col8, WHITE, get_cycle_count());
	// draw_image(gLogoBitmap);
    free(our_macOS_vers);
    free(our_version);
	return 0;
}

