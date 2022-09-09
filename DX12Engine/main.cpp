#include "ViewerApp.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR lpCmdLine, _In_ int nCmdShow)
{
    ViewerApp viewer(hInstance);
    viewer.Run();
    DEBUG_LOG("Exiting application");
    return 0;
}
