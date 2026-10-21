/* SPDX-License-Identifier: GPL-2.0-or-later. Host-only FIO adapter. */
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#define O_RDONLY 0
#define O_SYNC 0
static inline const char*get_config_dir(void){return "build/prefs/";}
static inline int FIO_GetFileSize(const char*p,uint32_t*n)
{struct stat s;if(stat(p,&s))return -1;*n=(uint32_t)s.st_size;return 0;}
static inline FILE*FIO_OpenFile(const char*p,int flags){(void)flags;return fopen(p,"rb");}
static inline FILE*FIO_CreateFile(const char*p){return fopen(p,"wb");}
static inline int FIO_ReadFile(FILE*f,void*p,unsigned n){return (int)fread(p,1,n,f);}
static inline int FIO_WriteFile(FILE*f,const void*p,unsigned n){return (int)fwrite(p,1,n,f);}
static inline int FIO_CloseFile(FILE*f){return fclose(f);}
