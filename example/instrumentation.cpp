
#include <iostream>
#include <execinfo.h>
#include <cxxabi.h>
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <dlfcn.h>
#include <link.h>

#include "tracer.hpp"

namespace
{
std::string resolve_name(void* address)
{
    Dl_info info;
    int status;

    // Attempt to resolve the symbol using dladdr
    if (dladdr(address, &info) && info.dli_sname)
    {
        // Demangle the symbol name if available
        char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        std::string result = (status == 0 && demangled) ? demangled : info.dli_sname;
        free(demangled);
        return result;
    }

    // If initial address fails, unwind the backtrace (up to 20 frames)
    constexpr int MAX_FRAMES = 20;
    void* addresses[MAX_FRAMES];
    int frame_count = backtrace(addresses, MAX_FRAMES);

    // skip frames because profiling functions are on top
    for (int i = 2; i < frame_count; ++i)
    {
        if (dladdr(addresses[i], &info) && info.dli_sname)
        {
            // Try to demangle the found symbol
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            std::string result = (status == 0 && demangled) ? demangled : info.dli_sname;
            free(demangled);
            return result;
        }
    }

    // If no symbol could be resolved, return the address in hex format
    std::stringstream ss;
    ss << std::hex << address;
    return ss.str();
}

// Callback function to get the base address of a shared library or executable
int getBaseAddress(struct dl_phdr_info* info, size_t size, void* data)
{
    // Find the first loadable segment and return its address as the base address
    if (info->dlpi_phnum > 0)
    {
        uintptr_t* baseAddress = static_cast<uintptr_t*>(data);
        *baseAddress = info->dlpi_addr;
        return 1;  // Stop iteration
    }
    return 0;  // Continue iteration
}

uintptr_t getBaseAddress()
{
    uintptr_t baseAddress = 0;
    dl_iterate_phdr(getBaseAddress, &baseAddress);
    return baseAddress;
}

}

extern "C" {

void __cyg_profile_func_enter(void* this_fn, void* call_site)
{
    uintptr_t baseAddress = getBaseAddress();
    uintptr_t adjustedAddress = reinterpret_cast<uintptr_t>(this_fn) - baseAddress;

    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0')
        << reinterpret_cast<uintptr_t>(adjustedAddress);
    // std::ostringstream oss;
    // oss << "0x" << std::hex << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0')
    //     << reinterpret_cast<uintptr_t>(this_fn);

    TRACE_BEGIN("PERF", oss.str());
    // TRACE_BEGIN("PERF", resolve_name(this_fn));
}

void __cyg_profile_func_exit(void* this_fn, void* call_site)
{
    uintptr_t baseAddress = getBaseAddress();
    uintptr_t adjustedAddress = reinterpret_cast<uintptr_t>(this_fn) - baseAddress;

    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0')
        << reinterpret_cast<uintptr_t>(adjustedAddress);
    // std::ostringstream oss;
    // oss << "0x" << std::hex << std::setw(sizeof(uintptr_t) * 2) << std::setfill('0')
    //     << reinterpret_cast<uintptr_t>(this_fn);

    TRACE_END("PERF", oss.str());
    // TRACE_END("PERF", resolve_name(this_fn));
}

}  // extern "C"