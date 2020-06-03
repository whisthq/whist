
#include "logging.h"

#ifdef _WIN32
#include <windows.h>
#include <D3D11.h>
#include <D3d11_1.h>
#include <DXGITYPE.h>
#include <dxgi1_2.h>
#pragma comment(lib, "dxguid.lib")
#include <processthreadsapi.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#endif

#include <stdio.h>

void PrintOSInfo()
{
    char buf[1024];

#ifdef _WIN32
    char product[256];
    char version[256];
    char buildlab[512];
    DWORD product_size = sizeof(product);
    DWORD version_size = sizeof( version );
    DWORD buildlab_size = sizeof( buildlab );
    RegGetValueA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", RRF_RT_ANY, NULL, &product, &product_size );
    RegGetValueA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentVersion", RRF_RT_ANY, NULL, &version, &version_size );
    RegGetValueA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "BuildLab", RRF_RT_ANY, NULL, &buildlab, &buildlab_size );

    LOG_INFO( "PROD NAME: %S", product );

    char winOSstring[512];
    snprintf( winOSstring, sizeof( winOSstring ), "%s %s.%s", product, version, buildlab );
#endif

#ifdef _WIN32
    snprintf( buf, sizeof( buf ), "32-bit %s", winOSstring );
#elif _WIN64
    snprintf( buf, sizeof( buf ), "64-bit %s", winOSstring );
#elif __APPLE__ || __MACH__
    buf = "Mac OSX";
#elif __linux__
    struct utsname uts;
    uname( &uts );
    snprintf( buf, sizeof(buf), "%s %s %s %s %s", uts.machine, uts.sysname, uts.nodename, uts.release, uts.version);
#else
    buf = "Other";
#endif

    LOG_INFO( "  OS: %s", buf );
}

void PrintModelInfo()
{
#ifdef _WIN32
    char* response = NULL;
    int total_sz = runcmd( "wmic computersystem get model,manufacturer", &response );
    if( response )
    {
        // Get rid of leading whitespace, we jump until right after the \n
        int find_newline = 0;
        while( find_newline < total_sz && response[find_newline] != '\n' ) find_newline++;
        find_newline++;
        char* make_model = response;
        make_model += find_newline;

        // Get rid of trailing whitespace
        int sz = (int)strlen( make_model );
        while( sz > 0 && make_model[sz - 1] == ' ' || make_model[sz - 1] == '\n' || make_model[sz - 1] == '\r' )
        {
            sz--;
        }
        make_model[sz] = '\0';

        // Get rid of consecutive spaces
        char* tmp = malloc( sz );
        for( int i = 1; i < sz; i++ )
        {
            if( make_model[i] == ' ' && make_model[i-1] == ' ' )
            {
                int target = i-1;
                int old_sz = sz;
                sz--;
                while( i + 1 < old_sz && make_model[i+1] == ' ' )
                {
                    i++;
                    sz--;
                }
                memcpy( tmp, &make_model[i], old_sz - i );
                memcpy( &make_model[target], tmp, old_sz - i );
                make_model[sz] = '\0';
                i--;
            }
        }
        free( tmp );

        // And now we print the new string
        LOG_INFO( "  Make and Model: %s", make_model );
        free( response );
    }
#else
    char* vendor = NULL;
    runcmd("cat /sys/devices/virtual/dmi/id/sys_vendor", &vendor);
    if(vendor) {
        vendor[strlen(vendor) - 1] = '\0';
        char* product = NULL;
        runcmd("cat /sys/devices/virtual/dmi/id/product_name", &product);
        if(product) {
            product[strlen(product) - 1] = '\0';
            LOG_INFO( "  Make and Model: %s %s", vendor, product );
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

void PrintMonitors()
{
#ifdef _WIN32
    int num_adapters = 0, i = 0, j = 0;
    IDXGIFactory1* factory;

#define MAX_NUM_ADAPTERS 10
    IDXGIAdapter1* adapters[MAX_NUM_ADAPTERS];

    HRESULT hr = CreateDXGIFactory1( &IID_IDXGIFactory1, (void**)(&factory) );
    if( FAILED( hr ) )
    {
        LOG_WARNING( "Failed CreateDXGIFactory1: 0x%X %d", hr, GetLastError() );
        return;
    }

    IDXGIAdapter1* adapter;
    // GET ALL GPUS
    while( factory->lpVtbl->EnumAdapters1( factory, num_adapters,
                                           &adapter ) !=
           DXGI_ERROR_NOT_FOUND )
    {
        if( num_adapters == MAX_NUM_ADAPTERS )
        {
            LOG_WARNING( "Too many adaters!\n" );
            break;
        }
        adapters[num_adapters] = adapter;
        ++num_adapters;
    }

    // GET GPU DESCRIPTIONS
    for( i = 0; i < num_adapters; i++ )
    {
        DXGI_ADAPTER_DESC1 desc;
        hr = adapters[i]->lpVtbl->GetDesc1( adapters[i], &desc );
        LOG_WARNING( "Adapter %d: %S", i, desc.Description );
    }

    LOG_INFO( "Monitor Info:" );

    // GET ALL MONITORS
    for( i = 0; i < num_adapters; i++ )
    {
        IDXGIOutput* output;
        for( j = 0;
             adapters[i]->lpVtbl->EnumOutputs(
                 adapters[i], j, &output ) != DXGI_ERROR_NOT_FOUND;
             j++ )
        {
            DXGI_OUTPUT_DESC output_desc;
            hr = output->lpVtbl->GetDesc( output, &output_desc );
            LOG_INFO( "  Found monitor %d on adapter %lu. Monitor %d named %S", j, i, j, output_desc.DeviceName );

            HMONITOR hMonitor = output_desc.Monitor;
            MONITORINFOEXW monitorInfo;
            monitorInfo.cbSize = sizeof( MONITORINFOEXW );
            GetMonitorInfoW( hMonitor, (LPMONITORINFO)&monitorInfo );
            DEVMODE devMode = { 0 };
            devMode.dmSize = sizeof( DEVMODE );
            devMode.dmDriverExtra = 0;
            EnumDisplaySettingsW( monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode );

            UINT dpiX, dpiY;
            hr = GetDpiForMonitor( hMonitor, MDT_DEFAULT, &dpiX, &dpiY );

            char* orientation = NULL;
            switch( devMode.dmDisplayOrientation )
            {
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
                LOG_WARNING( "Orientation did not match: %d", devMode.dmDisplayOrientation );
                orientation = "";
                break;
            }

            LOG_INFO( "  Resolution of %dx%d, Refresh Rate of %d, DPI %d, location (%d,%d), orientation %s", devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmDisplayFrequency, dpiX, devMode.dmPosition.x, devMode.dmPosition.y, orientation );
        }
    }
#endif
}

void PrintRAMInfo()
{
    size_t total_ram;
    size_t total_ram_usage;
#if defined(_WIN32)
    unsigned long long memory_in_kilos = 0;
    if( !GetPhysicallyInstalledSystemMemory( &memory_in_kilos ) )
    {
        LOG_WARNING( "Could not retrieve system memory: %d", GetLastError() );
    }
    total_ram = memory_in_kilos * 1024;

    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof( statex );

    GlobalMemoryStatusEx( &statex );

    total_ram_usage = total_ram - statex.ullAvailPhys;

    // Display the contents of the SYSTEM_INFO structure.
#else
    struct sysinfo info;
    sysinfo( &info );

    total_ram = info.totalram;
    total_ram_usage = info.totalram - (info.freeram + info.bufferram);
#endif
    LOG_INFO( "  Total RAM Usage: %.2f GB",
              total_ram_usage / 1024.0 / 1024.0 / 1024.0 );
    LOG_INFO( "  Total Physical RAM: %.2f GB",
              total_ram / 1024.0 / 1024.0 / 1024.0 );
}

void PrintMemoryInfo()
{
#if defined(_WIN32)
    DWORD processID = GetCurrentProcessId();
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;

    // Print information about the memory usage of the process.

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                            processID );
    if( NULL == hProcess ) return;

    if( GetProcessMemoryInfo( hProcess, &pmc, sizeof( pmc ) ) )
    {
        LOG_INFO( "PeakWorkingSetSize: %lld", (long long)pmc.PeakWorkingSetSize );
        LOG_INFO( "WorkingSetSize: %lld", (long long)pmc.WorkingSetSize );
    }

    CloseHandle( hProcess );
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

    if( strcmp( cpuVendor, "GenuineIntel" ) == 0 )
    {
        // Get DCP cache info
        cpuID( 4, regs );
        cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1

    } else if( strcmp( cpuVendor, "AuthenticAMD" ) == 0 )
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
    double used_space;
    double total_space;
    double available_space;
#ifdef _WIN32
    ULARGE_INTEGER ltotal_space;
    ULARGE_INTEGER lusable_space;
    ULARGE_INTEGER lfree_space;
    GetDiskFreeSpaceExW( NULL, &lusable_space, &ltotal_space, &lfree_space );

    used_space = (double)(ltotal_space.QuadPart - lfree_space.QuadPart);
    total_space = (double)ltotal_space.QuadPart;
    available_space = (double)lusable_space.QuadPart;
#else
    struct statvfs buf;
    statvfs("/", &buf);
    total_space = buf.f_blocks * buf.f_frsize;
    used_space = total_space - buf.f_bfree * buf.f_bsize;
    available_space = buf.f_bavail * buf.f_bsize;
#endif

    double BYTES_IN_GB = 1024 * 1024 * 1024.0;
    LOG_INFO( "  Hard Drive: %fGB/%fGB used, %fGB available to Fractal", used_space / BYTES_IN_GB, total_space / BYTES_IN_GB, available_space / BYTES_IN_GB );
}

