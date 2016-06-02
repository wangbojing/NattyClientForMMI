/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2016
 * 
 *
 
****       *****
  ***        *
  ***        *                         *               *
  * **       *                         *               *
  * **       *                         *               *
  *  **      *                        **              **
  *  **      *                       ***             ***
  *   **     *       ******       ***********     ***********    *****    *****
  *   **     *     **     **          **              **           **      **
  *    **    *    **       **         **              **           **      *
  *    **    *    **       **         **              **            *      *
  *     **   *    **       **         **              **            **     *
  *     **   *            ***         **              **             *    *
  *      **  *       ***** **         **              **             **   *
  *      **  *     ***     **         **              **             **   *
  *       ** *    **       **         **              **              *  *
  *       ** *   **        **         **              **              ** *
  *        ***   **        **         **              **               * *
  *        ***   **        **         **     *        **     *         **
  *         **   **        **  *      **     *        **     *         **
  *         **    **     ****  *       **   *          **   *          *
*****        *     ******   ***         ****            ****           *
                                                                       *
                                                                      *
                                                                  *****
                                                                  ****


 *
 */





#ifndef __NATTY_ABSTRACT_CLASS_H__
#define __NATTY_ABSTRACT_CLASS_H__

#include "kal_public_api.h"
#include "OslMemory_Int.h"
#include "MMIDataType.h"
//#include "mmidatatype.h"
#if 0
typedef long long U64;
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;
#endif

typedef long long C_DEVID;


typedef int (*HANDLE_CLIENTID)(void* client, C_DEVID id);
typedef int (*HANDLE_NOTIFY)(C_DEVID from, C_DEVID to);
typedef void (*HANDLE_TIMER)(void);
typedef PsIntFuncPtr HANDLE_RECV;

typedef struct {
	size_t size;
	void* (*ctor)(void *_self);
	void* (*dtor)(void *_self);
} AbstractClass;


void *New(const void *_class);
void Delete(void *_class);


#endif


