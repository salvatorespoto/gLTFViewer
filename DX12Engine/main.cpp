// SPDX-FileCopyrightText: 2022 Salvatore Spoto <salvatore.spoto@gmail.com> 
// SPDX-License-Identifier: MIT

#include "TinyEngine.h"

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR, int nCmdShow)
{
    TinyEngine engine(hInstance);
    engine.Run();

    DEBUG_LOG("Exiting application");
    return 0;
}
