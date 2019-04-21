// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
#import <Foundation/Foundation.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#else
#include <intrin.h>
#endif

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#include "defs.h"
#include "../http/httpclientconnection.h"

namespace {

#ifndef __APPLE__
    std::string external_cmd(const char *cmd) {
        char buffer[200];
        std::string result;
#ifdef _WIN32
        std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
#else
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
#endif
        if (pipe) {
            while (!feof(pipe.get()))
                if (fgets(buffer, sizeof buffer, pipe.get()) != nullptr)
                    result += buffer;
            auto pos = result.find_last_not_of(" \t\r\n");
            if (pos != std::string::npos)
                result.resize(pos+1);
        }
        return result;
    }
#endif

    std::string app_name() {

        std::string appName = "Bredbandskollen";

#ifdef _WIN32
        appName += " Windows";
#elif defined(__ANDROID__)
        appName += " Android";
#elif defined(__APPLE__)
        appName += " Mac";
#elif defined(__FreeBSD__)
        appName += " FreeBSD";
#elif defined(__NetBSD__)
        appName += " NetBSD";
#elif defined(__OpenBSD__)
        appName += " OpenBSD";
#elif defined(__bsdi__)
        appName += " BSDi";
#elif defined(__DragonFly__)
        appName += " DragonFly";
#else
        appName += " Linux";
#endif

#ifdef __arm__
        return appName + " ARM";
#elif defined(_M_ARM)
        return appName + " ARM";
#elif defined(__aarch64__)
        return appName + " ARM64";
#elif defined(__i386)
        return appName + " i386";
#elif defined(_M_IX86)
        return appName + " i386";
#elif defined(__X86__)
        return appName + " i386";
#elif defined(_X86_)
        return appName + " i386";
#elif defined(__amd64__)
        return appName + " amd64";
#elif defined(__x86_64__)
        return appName + " amd64";
#elif defined(_M_X64)
        return appName + " amd64";
#elif defined(_M_AMD64)
        return appName + " amd64";
#elif defined(__mips__)
        return appName + " mips";
#elif defined(__mips)
        return appName + " mips";
#else
        return appName;
#endif
    }

    std::string getHWModel() {
#ifdef __APPLE__
        size_t size;
        char array[1000];
        sysctlbyname("hw.model", NULL, &size, NULL, 0);
        sysctlbyname("hw.model", &array, &size, NULL, 0);
        return array;
#elif defined(_WIN32)
        // Get extended ids.
        int CPUInfo[4] = { -1 };
        __cpuid(CPUInfo, 0x80000000);
        unsigned int nExIds = CPUInfo[0];

        // Get the information associated with each extended ID.
        char CPUBrandString[0x40] = { 0 };
        for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
            __cpuid(CPUInfo, i);
            // Interpret CPU brand string and cache information.
            if (i == 0x80000002)
                memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));          // "Intel(R) Core(TM)"
            else if (i == 0x80000003)
                memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));     // "i7-4710HQ CPU"
            else if (i == 0x80000004)
                memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));     // "@ 2.50GHz"
        }
        return CPUBrandString;
#else
        return external_cmd("uname -m").substr(0, 50);
#endif
    }

    std::string getOSInfo() {
        std::string osInfo = "";
#ifdef _WIN32
        osInfo = "Windows ";

        NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
        OSVERSIONINFOEXW osVerInfo;

        *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

        if (NULL != RtlGetVersion) {
            osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
            RtlGetVersion(&osVerInfo);
            osInfo += std::to_string(osVerInfo.dwMajorVersion) + "." + std::to_string(osVerInfo.dwMinorVersion);
            if (osVerInfo.szCSDVersion[0] != 0) {
                osInfo += " ";
                int i = 0;
                while (osVerInfo.szCSDVersion[i] != 0) {
                    osInfo += osVerInfo.szCSDVersion[i];
                    i++;
                }
            }
            if (osVerInfo.wSuiteMask | VER_SUITE_PERSONAL) {
                osInfo += " Home";
            }
            osInfo += " (Build " + std::to_string(osVerInfo.dwBuildNumber) + ")";
        }
        else {
            osInfo += "unknown";
        }

#elif defined(__APPLE__)
        osInfo = [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];

#else
        osInfo = external_cmd("uname -sr").substr(0, 50);
#endif
        return osInfo;
    }
}

const std::string measurement::appName = app_name();
#if defined(__ANDROID__)
const std::string measurement::hw_info = "Android Device";
const std::string measurement::os_version = "Android OS";
#else
const std::string measurement::hw_info = getHWModel();
const std::string measurement::os_version = getOSInfo();
#endif
