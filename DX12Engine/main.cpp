#include "ViewerApp.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    ViewerApp viewer(hInstance);
    viewer.Run();
    return 0;
}
