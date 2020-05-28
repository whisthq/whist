/*
 * General Fractal helper functions and headers.
 *
 * Copyright Fractal Computers, Inc. 2020
 **/

#include "fractal.h"  // header file for this protocol, includes winsock
#include "../utils/json.h"

// Print Memory Info
#if defined(_WIN32)
#include "../video/dxgicapture.h"
#include <processthreadsapi.h>
#include <psapi.h>
#endif

void PrintRAMInfo()
{
#if defined(_WIN32)
    unsigned long long memory_in_kilos = 0;
    if( !GetPhysicallyInstalledSystemMemory( &memory_in_kilos ) )
    {
        LOG_WARNING( "Could not retrieve system memory: %d", GetLastError() );
    }

    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof( statex );

    GlobalMemoryStatusEx( &statex );

    size_t total_ram_usage = memory_in_kilos * 1024 - statex.ullAvailPhys;

    // Display the contents of the SYSTEM_INFO structure.
    LOG_INFO( "  Total RAM Usage: %.2f GB",
              total_ram_usage / 1024.0 / 1024.0 / 1024.0);
    LOG_INFO( "  Total Physical RAM: %.2f GB",
              memory_in_kilos / 1024.0 / 1024.0 );
#endif
}

void PrintMemoryInfo() {
#if defined(_WIN32)
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    // Print information about the memory usage of the process.

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                           processID);
    if (NULL == hProcess) return;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        LOG_INFO("PeakWorkingSetSize: %lld", (long long)pmc.PeakWorkingSetSize);
        LOG_INFO("WorkingSetSize: %lld", (long long)pmc.WorkingSetSize);
    }

    CloseHandle(hProcess);
#endif
}
// End Print Memory Info

void cpuID( unsigned i, unsigned regs[4] )
{
#ifdef _WIN32
    __cpuid( (int*)regs, (int)i );

#else
    asm volatile
        ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
          : "a" (i), "c" (0));
    // ECX is set to zero for CPUID function 4
#endif
}

void PrintCPUInfo()
{
    // https://stackoverflow.com/questions/2901694/how-to-detect-the-number-of-physical-processors-cores-on-windows-mac-and-linu
    unsigned regs[4];

    // Get vendor
    char cpuVendor[13] = { 0 };
    cpuID( 0, regs );
    ((unsigned*)cpuVendor)[0] = regs[1]; // EBX
    ((unsigned*)cpuVendor)[1] = regs[3]; // EDX
    ((unsigned*)cpuVendor)[2] = regs[2]; // ECX

    LOG_INFO( "  CPU Vendor: %s", cpuVendor );

    // Get Brand String
    unsigned int nExIds = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    cpuID( 0x80000000, regs );
    nExIds = regs[0];
    for( unsigned int i = 0x80000000; i <= nExIds; ++i )
    {
        cpuID( i, regs );
        // Interpret CPU brand string
        if( i == 0x80000002 )
            memcpy( CPUBrandString, regs, sizeof( regs ) );
        else if( i == 0x80000003 )
            memcpy( CPUBrandString + 16, regs, sizeof( regs ) );
        else if( i == 0x80000004 )
            memcpy( CPUBrandString + 32, regs, sizeof( regs ) );
    }
    //string includes manufacturer, model and clockspeed
    LOG_INFO( "  CPU Type: %s", CPUBrandString );

    // Logical core count per CPU
    cpuID( 1, regs );
    unsigned logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
    LOG_INFO( "  Logical Cores: %d", logical );
    unsigned cores = logical;

    if( strcmp(cpuVendor, "GenuineIntel") == 0 )
    {
        // Get DCP cache info
        cpuID( 4, regs );
        cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1

    } else if( strcmp(cpuVendor, "AuthenticAMD") == 0 )
    {
        // Get NC: Number of CPU cores - 1
        cpuID( 0x80000008, regs );
        cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
    } else
    {
        LOG_WARNING( "Unrecognized processor: %s", cpuVendor );
    }

    LOG_INFO( "  Physical Cores: %d", cores );

    // Get CPU features
    cpuID( 1, regs );
    unsigned cpuFeatures = regs[3]; // EDX

    // Detect hyper-threads  
    bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

    LOG_INFO( "  HyperThreaded: %s", (hyperThreads ? "true" : "false") );
}

void PrintHardDriveInfo()
{
#ifdef _WIN32
    ULARGE_INTEGER usable_space;
    ULARGE_INTEGER total_space;
    ULARGE_INTEGER free_space;
    GetDiskFreeSpaceExW( NULL, &usable_space, &total_space, &free_space );

    double BYTES_IN_GB = 1024 * 1024 * 1024.0;

    LOG_INFO( "  Hard Drive: %fGB/%fGB used, %fGB available to Fractal", (total_space.QuadPart - free_space.QuadPart) / BYTES_IN_GB, total_space.QuadPart / BYTES_IN_GB, usable_space.QuadPart / BYTES_IN_GB );
#endif
}

void PrintSystemInfo()
{
    LOG_INFO( "Hardware information:" );

    PrintCPUInfo();
    PrintRAMInfo();

#ifdef _WIN32

    char* response = NULL;
    int total_sz = runcmd( "wmic computersystem get model,manufacturer", &response );
    if( response )
    {
        // Get rid of leading whitespace, we jump until right after the \n
        int find_newline = 0;
        while( find_newline < total_sz && response[find_newline] != '\n' ) find_newline++;
        find_newline++;
        char* old_buf = response;
        response += find_newline;

        // Get rid of trailing whitespace
        int sz = (int)strlen( response );
        while( sz > 0 && response[sz - 1] == ' ' || response[sz - 1] == '\n' || response[sz - 1] == '\r' )
        {
            sz--;
        }
        response[sz] = '\0';

        // Get rid of consecutive spaces
        char* tmp = malloc( sz );
        for( int i = 1; i < sz; i++ )
        {
            if( response[i] == ' ' && response[i-1] == ' ' )
            {
                int target = i-1;
                int old_sz = sz;
                sz--;
                while( i + 1 < old_sz && response[i+1] == ' ' )
                {
                    i++;
                    sz--;
                }
                memcpy( tmp, &response[i], old_sz - i );
                memcpy( &response[target], tmp, old_sz - i );
                response[sz] = '\0';
                i--;
            }
        }
        free( tmp );

        // And now we print the new string
        LOG_INFO( "  Make and Model: %s", response);
        free( old_buf );
    }

    // Print Monitor
    PrintMonitors();
#endif

    PrintHardDriveInfo();
}

void runcmd_nobuffer(const char* cmdline) {
    // Will run a command on the commandline, simple as that
#ifdef _WIN32
    // Windows makes this hard
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    char cmd_buf[1000];

    if (strlen((const char*)cmdline) + 1 > sizeof(cmd_buf)) {
        mprintf("runcmd cmdline too long!\n");
        return;
    }

    memcpy(cmd_buf, cmdline, strlen((const char*)cmdline) + 1);

    SetEnvironmentVariableW((LPCWSTR)L"UNISON", (LPCWSTR)L"./.unison");

    if (CreateProcessA(NULL, (LPSTR)cmd_buf, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#else
    // For Linux / MACOSX we make the system syscall
    system(cmdline);
#endif
}

int runcmd(const char* cmdline, char** response) {
    if (response == NULL) {
        runcmd_nobuffer(cmdline);
        return 0;
    }

    FILE* pPipe;

    /* Run DIR so that it writes its output to a pipe. Open this
     * pipe with read text attribute so that we can read it
     * like a text file.
     */

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

    char* cmd = malloc(strlen(cmdline) + 128);
#ifdef _WIN32
    snprintf(cmd, strlen(cmdline) + 128, "%s 2>nul", cmdline);
#else
    snprintf(cmd, strlen(cmdline) + 128, "%s 2>/dev/null", cmdline);
#endif

    if ((pPipe = popen(cmd, "r")) == NULL) {
        LOG_WARNING( "Failed to popen %s", cmd );
        free(cmd);
        return -1;
    }
    free(cmd);

    /* Read pipe until end of file, or an error occurs. */

    int current_len = 0;

    int max_len = 128;
    char* buffer = malloc(max_len);

    while (true) {

        char c = (char)fgetc(pPipe);
        if (current_len == max_len) {
            int next_max_len = 2 * max_len;
            buffer = realloc(buffer, next_max_len);

            max_len = next_max_len;
        }

        if (c == EOF) {
            buffer[current_len] = '\0';
            break;
        } else {
            buffer[current_len] = c;
            current_len++;
        }
    }

    *response = buffer;

    /* Close pipe and print return value of pPipe. */
    if (feof(pPipe)) {
        return current_len;
    } else {
        LOG_WARNING("Error: Failed to read the pipe to the end.\n");
        return -1;
    }
}

char* get_ip() {
    static char ip[128];
    static bool already_obtained_ip = false;
    if (already_obtained_ip) {
        return ip;
    }

    char* buf;
    int len = runcmd("curl ipinfo.io", &buf);
    len;

    json_t json;
    if (!parse_json(buf, &json)) {
        LOG_WARNING("curl ipinfo.io did not return an IP: %s", buf);
        return NULL;
    }
    kv_pair_t* kv = get_kv(&json, "ip");

    memcpy(ip, kv->str_value, sizeof(ip));

    free_json(json);

    already_obtained_ip = true;
    return ip;
}

static char* branch = NULL;
static bool is_dev;
static bool already_obtained_vm_type = false;

char* get_branch() {
    is_dev_vm();
    return branch;
}

bool is_dev_vm() {
    if (already_obtained_vm_type) {
        return is_dev;
    }

    char buf[4800];
    size_t len = sizeof(buf);

    LOG_INFO("GETTING JSON");

    if (!SendJSONGet("cube-celery-staging.herokuapp.com", "/vm/isDev", buf,
                     len)) {
        return true;
    }

    char* json_str = NULL;
    for (size_t i = 0; i < len - 4; i++) {
        if (memcmp(buf + i, "\r\n\r\n", 4) == 0) {
            json_str = buf + i + 4;
        }
    }

    json_t json;
    if( !parse_json( json_str, &json ) )
    {
        LOG_WARNING( "Failed to parse JSON from /vm/isDev" );
        already_obtained_vm_type = true;
        is_dev = true;
        return is_dev;
    }

    kv_pair_t* dev_value = get_kv(&json, "dev");
    kv_pair_t* branch_value = get_kv(&json, "branch");
    if (dev_value && branch_value) {
        if (dev_value->type != JSON_BOOL) {
            return false;
        }

        is_dev = dev_value->bool_value;
        branch = clone(branch_value->str_value);

        LOG_INFO("Is Dev? %s", dev_value->bool_value ? "true" : "false");
        LOG_INFO("Branch: %s", branch);

        free_json(json);

        already_obtained_vm_type = true;
        return is_dev;
    } else {
        free_json(json);
        return true;
    }
}

int GetFmsgSize(struct FractalClientMessage* fmsg) {
    if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
        return sizeof(*fmsg);
    } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
        return sizeof(*fmsg) + fmsg->clipboard.size;
    } else {
        return sizeof(fmsg->type) + 40;
    }
}
