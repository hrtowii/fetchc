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
	char* our_version = malloc(256);
    size_t size = 256;
	if (sysctlbyname("kern.version", our_version, &size, NULL, 0) != 0) {
		strcpy(our_version, "unknown");
	};
	return our_version;
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
	char hostname[1024];
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

uint32_t gLogoBitmap[32] = { 0x0, 0xa00, 0x400, 0x5540, 0x7fc0, 0x3f80, 0x3f80, 0x1f00, 0x1f00, 0x1f00, 0x3f80, 0xffe0, 0x3f80, 0x3f80, 0x3f83, 0x103f9f, 0x18103ffb, 0xe3fffd5, 0x1beabfab, 0x480d7fd5, 0xf80abfab, 0x480d7fd5, 0x1beabfab, 0xe3fffd5, 0x18107ffb, 0x107fdf, 0x7fc3, 0xffe0, 0xffe0, 0xffe0, 0x1fff0, 0x1fff0 };
#define IMAGE_HEIGHT 32
#define IMAGE_WIDTH 1
int shrink_bitmap_to_8x8(uint32_t *src, uint32_t *dest) {
    for (int y = 0; y < 8; y++) {
        uint32_t row = 0;
        for (int x = 0; x < 8; x++) {
            uint32_t pixel = src[y * 32 + x];
            row |= (pixel & 0x80000000) >> (31 - x);
        }
        dest[y] = row;
    }
	return 1;
}
void print_bitmap_array_hex(uint32_t *data, int height, int width) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t pixel = data[y * width + x];
            printf("0x%08X ", pixel);
        }
        printf("\n");
    }
}

void draw_image(uint32_t *data) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            uint32_t pixel = data[y * IMAGE_WIDTH + x];
            for (int bit = 0; bit < 32; bit++) {
                if ((pixel >> bit) & 1) {
                    printf("█");
                } else {
                    printf(" ");
                }
            }
        }
        printf("\n"); // Move to the next line after each row
    }
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
	printf("%s", info.col1);
	printf("%s\n", info.col2);
	printf("%s HOSTNAME  %s%s\n", info.col3, WHITE, our_hostname);
	print_uptime(our_uptime);
	printf("%s macOS	   %s%s\n%s KERNEL    %s%s\n", info.col5, WHITE, our_macOS_vers, info.col6, WHITE, our_version);
	free(our_version);
	printf("%s INSTALLED %s%d\n", info.col7, WHITE, get_installed_packages_brew());
	printf("%s RAM	   %s%3.1f MiB / %3.1f MiB\n", info.col8, WHITE, get_used_ram(), get_total_ram());
	// draw_image(gLogoBitmap);
	return 0;
}

