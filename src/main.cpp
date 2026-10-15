#include "app/application.h"

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previous,
    _In_ PWSTR commandLine, _In_ int showCmd) {
    return attendance::RunApplication(instance, previous, commandLine, showCmd);
}
