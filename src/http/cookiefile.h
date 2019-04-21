// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <map>

#include "../http/cookiemanager.h"

class CookieFile : public CookieManager {
public:
    CookieFile(const std::string &cookie_filename = "") :
    CookieManager("CookieFile"),
    filename(cookie_filename) {
        readCookiesFile();
    }

    // By default, we try to save cookies in the destructor. However, if
    // the save operation fails, all updates are lost. If you can't afford to
    // lose cookies, call save() and check the return value.
    ~CookieFile() {
        if (isDirty())
            writeCookiesFile();
    }

    // Write to disk, return false on failure.
    bool save() override {
        if (isDirty())
            writeCookiesFile();
        return isDirty();
    }

    // Default move constructor is OK despite us having a destructor:
    CookieFile(CookieFile &&) = default;

private:
    void readCookiesFile();
    void writeCookiesFile();
    std::string filename;
};
