#include <iostream>
#include <windows.h>
#include "rtt_jlink.hpp"

// using namespace std;

// typedef void(*ptrOpen)(void);
// typedef int(*ptrIsOpened)(void);

// HINSTANCE hMod = LoadLibrary(TEXT("C:\\tools\\SEGGER\\JLink_V768a\\JLink_x64"));

// int main()
// {
//     if (hMod != NULL) 
//     {
//         ptrOpen JLINKARM_Open = (ptrOpen)GetProcAddress(hMod, "JLINKARM_Open");
//         if (JLINKARM_Open != NULL)
//         {
//             JLINKARM_Open();
//         }
//         else
//         {
//             printf("NULL");
//         }

//         ptrIsOpened JLINKARM_IsOpen = (ptrIsOpened)GetProcAddress(hMod, "JLINKARM_IsOpen");
//         if(JLINKARM_IsOpen == NULL)
//         {
//             printf("NULL");
//         }
//         else
//         {
//             printf("%d", JLINKARM_IsOpen());
//         }
//         FreeLibrary(hMod);
//         /*在完成调用功能后，不在需要DLL支持，则可以通过FreeLibrary函数释放DLL。*/
//     }
//     else
//     {
//         printf("DLL load failed.");
//     }
//     // system("pause");

//     return 0;
// }

// void MainWindow::test()
// {
//     QLibrary mylib("C:\\tools\\SEGGER\\JLink_V768a\\JLink_x64.dll");
//     if(mylib.load())
//     {
//         qDebug() << "dll加载成功";
//         SetKeyBoardHook open = (SetKeyBoardHook)mylib.resolve("SetKeyBoardHook");
//         if(open)
//         {
//             qDebug() << "SetKeyBoardHook加载成功";
//             bool flag = open((HWND)this->winId());
//         }
//     }
// }
