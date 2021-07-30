/**
 * Copyright Fractal Computers, Inc. 2020
 * @file sysinfo.c
 * @brief This file contains all code that logs client hardware specifications.
============================
Usage
============================

Call the respective functions to log a device's OS, model, CPU, RAM, etc.
*/

#include "sysinfo.h"

/*
============================
Includes
============================
*/

#ifdef _WIN32
#pragma warning(disable : 4201)
#include <D3D11.h>
#include <D3d11_1.h>
#include <DXGITYPE.h>
#include <dxgi1_2.h>
#include <windows.h>
#pragma comment(lib, "dxguid.lib")
#include <processthreadsapi.h>
#include <psapi.h>
#else
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <sys/sysinfo.h>
#endif
#include <sys/statvfs.h>
#include <sys/utsname.h>
#endif

#include <stdio.h>

#include <fractal/logging/logging.h>

void print_os_info() {
#ifdef _WIN32
    char buf[1024];
    char product[256];
    char version[256];
    char buildlab[512];
    DWORD product_size = sizeof(product);
    DWORD version_size = sizeof(version);
    DWORD buildlab_size = sizeof(buildlab);
    LSTATUS ret;
    ret = RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       "ProductName", RRF_RT_ANY, NULL, &product, &product_size);
    if (ret != ERROR_SUCCESS) {
        LOG_INFO("ERROR: %ll", ret);
        product[0] = '\0';
    }
    ret = RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       "CurrentVersion", RRF_RT_ANY, NULL, &version, &version_size);
    if (ret != ERROR_SUCCESS) {
        LOG_INFO("ERROR: %ll", ret);
        version[0] = '\0';
    }
    ret = RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                       "BuildLab", RRF_RT_ANY, NULL, &buildlab, &buildlab_size);
    if (ret != ERROR_SUCCESS) {
        LOG_INFO("ERROR: %ll", ret);
        buildlab[0] = '\0';
    }

    char win_o_sstring[512];
    snprintf(win_o_sstring, sizeof(win_o_sstring), "%s %s.%s", product, version, buildlab);
#endif

#ifdef _WIN32
    snprintf(buf, sizeof(buf), "32-bit %s", win_o_sstring);
    LOG_INFO("  OS: %s", buf);
#elif _WIN64
    snprintf(buf, sizeof(buf), "64-bit %s", winOSstring);
    LOG_INFO("  OS: %s", buf);
#elif __APPLE__ || __MACH__
    char* os_version = NULL;
    runcmd("sw_vers", &os_version);
    char* token = strtok(os_version, "\n");
    LOG_INFO("OS %s", token);    // Get ProductName
    token = strtok(NULL, "\n");  // Get ProductVersion
    LOG_INFO("OS %s", token);
    token = strtok(NULL, "\n");  // Get BuildVersion
    LOG_INFO("OS %s", token);
#elif __linux__
    char buf[1024];
    struct utsname uts;
    uname(&uts);
    snprintf(buf, sizeof(buf), "%s %s %s %s %s", uts.machine, uts.sysname, uts.nodename,
             uts.release, uts.version);
    LOG_INFO("  OS: %s", buf);
#else
    snprintf(buf, sizeof(buf), "Other");
    LOG_INFO("  OS: %s", buf);
#endif
}

void print_model_info() {
#ifdef _WIN32
    char* response = NULL;
    int total_sz = runcmd("wmic computersystem get model,manufacturer", &response);
    if (response) {
        // Get rid of leading whitespace, we jump until right after the \n
        int find_newline = 0;
        while (find_newline < total_sz && response[find_newline] != '\n') find_newline++;
        find_newline++;
        char* make_model = response;
        make_model += find_newline;

        // Get rid of trailing whitespace
        int sz = (int)strlen(make_model);
        while (sz > 0 && (make_model[sz - 1] == ' ' || make_model[sz - 1] == '\n' ||
                          make_model[sz - 1] == '\r')) {
            sz--;
        }
        make_model[sz] = '\0';

        // Get rid of consecutive spaces
        char* tmp = safe_malloc(sz);
        for (int i = 1; i < sz; i++) {
            if (make_model[i] == ' ' && make_model[i - 1] == ' ') {
                int target = i - 1;
                int old_sz = sz;
                sz--;
                while (i + 1 < old_sz && make_model[i + 1] == ' ') {
                    i++;
                    sz--;
                }
                memcpy(tmp, &make_model[i], old_sz - i);
                memcpy(&make_model[target], tmp, old_sz - i);
                make_model[sz] = '\0';
                i--;
            }
        }
        free(tmp);

        // And now we print the new string
        LOG_INFO("  Make and Model: %s", make_model);
        free(response);
    }
#elif __APPLE__
    size_t len = 0;
    sysctlbyname("hw.model", NULL, &len, NULL, 0);
    if (len) {
        char* model = safe_malloc(len * sizeof(char));
        sysctlbyname("hw.model", model, &len, NULL, 0);
        LOG_INFO("  Make and Model: %s", model);
        free(model);
    }
#else
    char* vendor = NULL;
    runcmd("cat /sys/devices/virtual/dmi/id/sys_vendor", &vendor);
    if (vendor) {
        vendor[strlen(vendor) - 1] = '\0';
        char* product = NULL;
        runcmd("cat /sys/devices/virtual/dmi/id/product_name", &product);
        if (product) {
            product[strlen(product) - 1] = '\0';
            LOG_INFO("  Make and Model: %s %s", vendor, product);
            free(product);
        } else {
            LOG_WARNING("PRODUCT FAILED");
        }
        free(vendor);
    } else {
        LOG_WARNING("SYS VENDOR FAILED");
    }
#endif
}

void print_monitors() {
#ifdef _WIN32
    int num_adapters = 0, i = 0, j = 0;
    IDXGIFactory1* factory;

#define MAX_NUM_ADAPTERS 10
    IDXGIAdapter1* adapters[MAX_NUM_ADAPTERS];

    HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void**)(&factory));
    if (FAILED(hr)) {
        LOG_WARNING("Failed CreateDXGIFactory1: 0x%X %d", hr, GetLastError());
        return;
    }

    IDXGIAdapter1* adapter;
    // GET ALL GPUS
    while (factory->lpVtbl->EnumAdapters1(factory, num_adapters, &adapter) !=
           DXGI_ERROR_NOT_FOUND) {
        if (num_adapters == MAX_NUM_ADAPTERS) {
            LOG_WARNING("Too many adaters!\n");
            break;
        }
        adapters[num_adapters] = adapter;
        ++num_adapters;
    }

    // GET GPU DESCRIPTIONS
    for (i = 0; i < num_adapters; i++) {
        DXGI_ADAPTER_DESC1 desc;
        hr = adapters[i]->lpVtbl->GetDesc1(adapters[i], &desc);
        LOG_INFO("Adapter %d: %S", i, desc.Description);
    }

    LOG_INFO("Monitor Info:");

    // GET ALL MONITORS
    for (i = 0; i < num_adapters; i++) {
        IDXGIOutput* output;
        for (j = 0;
             adapters[i]->lpVtbl->EnumOutputs(adapters[i], j, &output) != DXGI_ERROR_NOT_FOUND;
             j++) {
            DXGI_OUTPUT_DESC output_desc;
            hr = output->lpVtbl->GetDesc(output, &output_desc);
            LOG_INFO("Found monitor %d on adapter %lu. Monitor %d named %S", j, i, j,
                     output_desc.DeviceName);

            HMONITOR h_monitor = output_desc.Monitor;
            MONITORINFOEXW monitor_info;
            monitor_info.cbSize = sizeof(MONITORINFOEXW);
            GetMonitorInfoW(h_monitor, (LPMONITORINFO)&monitor_info);
            DEVMODE dev_mode = {0};
            dev_mode.dmSize = sizeof(DEVMODE);
            dev_mode.dmDriverExtra = 0;
            EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode);

            UINT dpi_x, dpi_y;
            hr = GetDpiForMonitor(h_monitor, MDT_DEFAULT, &dpi_x, &dpi_y);

            char* orientation = NULL;
            switch (dev_mode.dmDisplayOrientation) {
                case DMDO_DEFAULT:
                    orientation = "default";
                    break;
                case DMDO_90:
                    orientation = "90 degrees";
                    break;
                case DMDO_180:
                    orientation = "180 degrees";
                    break;
                case DMDO_270:
                    orientation = "270 degrees";
                    break;
                default:
                    LOG_WARNING("Orientation did not match: %d", dev_mode.dmDisplayOrientation);
                    orientation = "";
                    break;
            }

            LOG_INFO(
                "Resolution of %dx%d, Refresh Rate of %d, DPI %d, location "
                "(%d,%d), orientation %s",
                dev_mode.dmPelsWidth, dev_mode.dmPelsHeight, dev_mode.dmDisplayFrequency, dpi_x,
                dev_mode.dmPosition.x, dev_mode.dmPosition.y, orientation);
        }
    }
#endif
}

void print_ram_info() {
#if defined(_WIN32)
    size_t total_ram;
    size_t total_ram_usage;
    unsigned long long memory_in_kilos = 0;
    if (!GetPhysicallyInstalledSystemMemory(&memory_in_kilos)) {
        LOG_WARNING("Could not retrieve system memory: %d", GetLastError());
    }
    total_ram = memory_in_kilos * 1024;

    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);

    GlobalMemoryStatusEx(&statex);

    total_ram_usage = total_ram - statex.ullAvailPhys;

    // MacOS: use sysctl
#elif __APPLE__
    unsigned long long total_ram;
    char* total_ram_usage = NULL;
    char* memsize = NULL;
    runcmd("sysctl hw.memsize", &memsize);
    char* token = strtok(memsize, ": ");
    token = strtok(NULL, ": ");       // get the second token, the RAM number
    token[strlen(token) - 1] = '\0';  // remove newline
    total_ram = atoll(token);
    runcmd("top -l 1 | grep -E '^Phys'", &total_ram_usage);
    total_ram_usage[strlen(total_ram_usage) - 1] = '\0';  // remove newline
    // LINUX: Display the contents of the SYSTEM_INFO structure.
#else
    size_t total_ram;
    size_t total_ram_usage;
    struct sysinfo info;
    sysinfo(&info);

    total_ram = info.totalram;
    total_ram_usage = info.totalram - (info.freeram + info.bufferram);
#endif
#ifndef __APPLE__
    LOG_INFO("Total RAM Usage: %.2f GB", total_ram_usage / BYTES_IN_GB);
#else
    LOG_INFO("Total RAM Usage: %s", total_ram_usage);
#endif
    LOG_INFO("Total Physical RAM: %.2f GB", (size_t)total_ram / BYTES_IN_GB);
}

void print_memory_info() {
#if defined(_WIN32)
    DWORD process_id = GetCurrentProcessId();
    HANDLE h_process;
    PROCESS_MEMORY_COUNTERS pmc;

    // Print information about the memory usage of the process.

    h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (NULL == h_process) return;

    if (GetProcessMemoryInfo(h_process, &pmc, sizeof(pmc))) {
        LOG_INFO("PeakWorkingSetSize: %lld", (long long)pmc.PeakWorkingSetSize);
        LOG_INFO("WorkingSetSize: %lld", (long long)pmc.WorkingSetSize);
    }

    CloseHandle(h_process);
#endif
}
// End Print Memory Info

#ifdef _WIN32
// Get __cpuid compiler intrinsic
#include <intrin.h>
#endif

#if defined __arm64__ && defined __APPLE__
#define M1
#endif

#ifndef M1
// On x86 processors, we can use cpuid function call
void cpu_id(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
    __cpuid((int*)regs, (int)i);
#else
    asm volatile("cpuid"
                 : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                 : "a"(i), "c"(0));
    // ECX is set to zero for CPUID function 4
#endif
}
#endif

void print_cpu_info() {
#ifdef M1
    const char* cpu_vendor = "AppleM1";
    const char* cpu_brand_string = "Apple M1 8-Core @ 3.2GHz";
    unsigned logical = 8;
    unsigned cores = 8;
    bool hyper_threads = false;
#else
    // https://stackoverflow.com/questions/2901694/how-to-detect-the-number-of-physical-processors-cores-on-windows-mac-and-linu
    unsigned regs[4];

    // Get vendor
    char cpu_vendor[13] = {0};
    cpu_id(0, regs);
    ((unsigned*)cpu_vendor)[0] = regs[1];  // EBX
    ((unsigned*)cpu_vendor)[1] = regs[3];  // EDX
    ((unsigned*)cpu_vendor)[2] = regs[2];  // ECX

    // Get Brand String
    unsigned int n_ex_ids = 0;
    char cpu_brand_string[0x40];
    // Get the information associated with each extended ID.
    cpu_id(0x80000000, regs);
    n_ex_ids = regs[0];
    for (unsigned int i = 0x80000000; i <= n_ex_ids; ++i) {
        cpu_id(i, regs);
        // Interpret CPU brand string
        if (i == 0x80000002)
            memcpy(cpu_brand_string, regs, sizeof(regs));
        else if (i == 0x80000003)
            memcpy(cpu_brand_string + 16, regs, sizeof(regs));
        else if (i == 0x80000004)
            memcpy(cpu_brand_string + 32, regs, sizeof(regs));
    }

    // Logical core count per CPU
    cpu_id(1, regs);
    unsigned logical = (regs[1] >> 16) & 0xff;  // EBX[23:16]
    unsigned cores = logical;

    if (strcmp(cpu_vendor, "GenuineIntel") == 0) {
        // Get DCP cache info
        cpu_id(4, regs);
        cores = ((regs[0] >> 26) & 0x3f) + 1;  // EAX[31:26] + 1

    } else if (strcmp(cpu_vendor, "AuthenticAMD") == 0) {
        // Get NC: Number of CPU cores - 1
        cpu_id(0x80000008, regs);
        cores = ((unsigned)(regs[2] & 0xff)) + 1;  // ECX[7:0] + 1
    } else {
        LOG_WARNING("Unrecognized processor: %s", cpu_vendor);
    }

    // Get CPU features
    cpu_id(1, regs);
    unsigned cpu_features = regs[3];  // EDX

    // Detect hyper-threads
    bool hyper_threads = cpu_features & (1 << 28) && cores < logical;
#endif

    // string includes manufacturer, model and clockspeed
    LOG_INFO("CPU Vendor: %s", cpu_vendor);
    LOG_INFO("CPU Type: %s", cpu_brand_string);
    LOG_INFO("Logical Cores: %d", logical);
    LOG_INFO("Physical Cores: %d", cores);
    LOG_INFO("HyperThreaded: %s", (hyper_threads ? "true" : "false"));

// add CPU usage at beginning of Fractal
#ifdef __APPLE__
    char* cpu_usage = NULL;
    runcmd("top -l 1 | grep -E '^CPU'", &cpu_usage);
    cpu_usage[strlen(cpu_usage) - 1] = '\0';  // remove newline
    LOG_INFO("  %s", cpu_usage);
#endif
}

void print_hard_drive_info() {
    double used_space;
    double total_space;
    double available_space;
#ifdef _WIN32
    ULARGE_INTEGER ltotal_space;
    ULARGE_INTEGER lusable_space;
    ULARGE_INTEGER lfree_space;
    GetDiskFreeSpaceExW(NULL, &lusable_space, &ltotal_space, &lfree_space);

    used_space = (double)(ltotal_space.QuadPart - lfree_space.QuadPart);
    total_space = (double)ltotal_space.QuadPart;
    available_space = (double)lusable_space.QuadPart;
#else
    struct statvfs buf;
    statvfs("/", &buf);
    total_space = buf.f_blocks * buf.f_frsize;
    used_space = total_space - buf.f_bfree * buf.f_frsize;
    available_space = buf.f_bavail * buf.f_frsize;
#endif
    LOG_INFO("Hard Drive: %.2fGB/%.2fGB used, %.2fGB available to Fractal",
             used_space / BYTES_IN_GB, total_space / BYTES_IN_GB, available_space / BYTES_IN_GB);
}
