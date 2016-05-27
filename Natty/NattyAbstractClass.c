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





#include <string.h>
#include <stdio.h>

#include "NattyAbstractClass.h"

void *New(const void *_class) {
	const AbstractClass *class = _class;
#ifdef __PLATFORM_LINUX_
	void *p = calloc(class->size);
#else
	void *p = OslMalloc(class->size);
#endif
	memset(p, 0, class->size);
	
	assert(p);
	*(const AbstractClass**)p = class;
	
	if (class->ctor) {
		p = class->ctor(p);
	}
	return p;
}


void Delete(void *_class) {
	const AbstractClass **class = _class;

	if (_class && (*class) && (*class)->dtor) {
		_class = (*class)->dtor(_class);
	}
#ifdef __PLATFORM_LINUX_
	free(class->size);
#else
	OslMfree(_class);
#endif	
}

