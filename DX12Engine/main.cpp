#include "WindowApp.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    WindowApp wApp(hInstance);
    wApp.init();
    wApp.run();
    return 0;
}
