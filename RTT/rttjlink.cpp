#include "rttjlink.hpp"

RTTJLink::RTTJLink(QObject *parent) : QObject(parent)
{
    // HMODULE hJLinkDll = LoadLibrary(TEXT("C:\\tools\\SEGGER\\JLink_V768a\\JLink_x64.dll"));

    // if(hJLinkDll == nullptr)
    //     qDebug() << "null";
    // else
    // {
    //     qDebug() << "ok";
    // }

   QLibrary mylib("C:/tools/SEGGER/JLink_V768a/JLinkARM.dll");
   if(mylib.load())
   {
       qDebug() << "dll加载成功";
       _ptrOpen open = (_ptrOpen)mylib.resolve("JLINKARM_Open");
       if(open)
       {
           qDebug() << "open函数加载成功";
        //    open();
       }
   }
   else
   {
       qDebug() << "dll加载失败";
   }
}

