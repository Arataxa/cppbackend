#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#endif

#include <boost/asio.hpp>

#include "Application.h"
#include "audio.h"

using namespace std::literals;

int main(int argc, char** argv) {
    Application application;

    application.Run();

    return 0;
}
