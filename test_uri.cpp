﻿#include "uri.h"
#include <iostream>

int main(int argc, char** argv) {
    Framework::Uri::ptr uri = Framework::Uri::CreateUri("http://www.baidu.com/test/uri?id=100&name=sylar#frg");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddressFromUri();
    std::cout << addr->toString() << std::endl;
    return 0;
}