// �汾�ſ����ļ�

#ifndef _VERSION_H_
#define _VERSION_H_

#define _VERTOSTRING(arg) #arg  
#define VERTOSTRING(arg) _VERTOSTRING(arg) 
#define VER_MAIN    0  
#define VER_SUB     1  
#define VER_SUB2    5 
#define VER_BUILD   11  
#define VER_FULL    VERTOSTRING(VER_MAIN.VER_SUB.VER_SUB2)
#define VER_FULL_RC VER_MAIN,VER_SUB,VER_SUB2,VER_BUILD

#endif // _VERSION_H_
