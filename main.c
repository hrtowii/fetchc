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

void print_uptime(time_t uptime) { // i begged claude for this
    time_t now = time(NULL);
    time_t boot_time = now - uptime;
    struct tm* boot_tm = gmtime(&boot_time);

    printf("%s UPTIME    %s", info.col4, WHITE);
    if (boot_tm->tm_year != 70) {
        printf("%d years, ", boot_tm->tm_year + 1900);
    }
    printf("%d days, %02d:%02d:%02d\n", boot_tm->tm_yday, boot_tm->tm_hour, boot_tm->tm_min, boot_tm->tm_sec);
}

char* get_version() {
    char our_version[256];
    size_t size = sizeof(our_version);
    if (sysctlbyname("kern.version", our_version, &size, NULL, 0) != 0) {
        char* unknown_version = strdup("unknown");
        return unknown_version;
    } else {
        char* colon = strchr(our_version, ':');
        if (colon != NULL) {
            *colon = '\0';
        }
        char* version_copy = strdup(our_version);
        return version_copy;
    }
}

char* get_macOS() { // asked claude dot ai for this.
    char buffer[128];
    FILE* pipe = popen("sw_vers -productVersion", "r");
    if (!pipe) {
        return NULL;
    }
    char* version = fgets(buffer, sizeof(buffer), pipe);
    pclose(pipe);
    if (version) {
        version[strcspn(version, "\n")] = '\0';
        return strdup(version);
    } else {
        return NULL;
    }
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
	info.col1 = BMAGENTA "            \n";
	info.col2 = BMAGENTA "  \\\\  \\\\ //     ";
	info.col3 = BMAGENTA " ==\\\\__\\\\/ //   ";
	info.col4 = BMAGENTA "   //   \\\\//    ";
	info.col5 = BMAGENTA "==//     //==   ";
	info.col6 = BMAGENTA " //\\\\___//      ";
	info.col7 = BMAGENTA "// /\\\\  \\\\==    ";
	info.col8 = BMAGENTA "  // \\\\  \\\\     ";

	char* our_version = get_version();
	time_t our_uptime = get_boot_time();
	char* our_hostname = get_hostname();
	char* our_macOS_vers = get_macOS();
	printf("%s", 										info.col1);
	printf("%s\n", 										info.col2);
	printf("%s HOSTNAME  %s%s\n", 						info.col3, WHITE, our_hostname);
	print_uptime(our_uptime);
	printf("%s macOS	   %s%s\n%s KERNEL    %s%s\n", 	info.col5, WHITE, our_macOS_vers, info.col6, WHITE, our_version);
	printf("%s INSTALLED %s%d\n", 						info.col7, WHITE, get_installed_packages_brew());
	printf("%s RAM	   %s%3.1f MiB / %3.1f MiB\n", 		info.col8, WHITE, get_used_ram(), get_total_ram());
	// draw_image(gLogoBitmap);
	return 0;
}

