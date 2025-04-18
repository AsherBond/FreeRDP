/**
 * WinPR: Windows Portable Runtime
 * Synchronization Functions
 *
 * Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <winpr/config.h>
#include <winpr/debug.h>
#include <winpr/synch.h>

#include "synch.h"

#ifdef WINPR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _WIN32

#include <errno.h>
#include "../handle/handle.h"
#include "../log.h"
#define TAG WINPR_TAG("synch.semaphore")

static BOOL SemaphoreCloseHandle(HANDLE handle);

static BOOL SemaphoreIsHandled(HANDLE handle)
{
	return WINPR_HANDLE_IS_HANDLED(handle, HANDLE_TYPE_SEMAPHORE, FALSE);
}

static int SemaphoreGetFd(HANDLE handle)
{
	WINPR_SEMAPHORE* sem = (WINPR_SEMAPHORE*)handle;

	if (!SemaphoreIsHandled(handle))
		return -1;

	return sem->pipe_fd[0];
}

static DWORD SemaphoreCleanupHandle(HANDLE handle)
{
	WINPR_SEMAPHORE* sem = (WINPR_SEMAPHORE*)handle;

	if (!SemaphoreIsHandled(handle))
		return WAIT_FAILED;

	uint8_t val = 0;
	const SSIZE_T length = read(sem->pipe_fd[0], &val, sizeof(val));

	if (length != 1)
	{
		char ebuffer[256] = { 0 };
		WLog_ERR(TAG, "semaphore read() failure [%d] %s", errno,
		         winpr_strerror(errno, ebuffer, sizeof(ebuffer)));
		return WAIT_FAILED;
	}

	return WAIT_OBJECT_0;
}

BOOL SemaphoreCloseHandle(HANDLE handle)
{
	WINPR_SEMAPHORE* semaphore = (WINPR_SEMAPHORE*)handle;

	if (!SemaphoreIsHandled(handle))
		return FALSE;

#ifdef WINPR_PIPE_SEMAPHORE

	if (semaphore->pipe_fd[0] != -1)
	{
		close(semaphore->pipe_fd[0]);
		semaphore->pipe_fd[0] = -1;

		if (semaphore->pipe_fd[1] != -1)
		{
			close(semaphore->pipe_fd[1]);
			semaphore->pipe_fd[1] = -1;
		}
	}

#else
#if defined __APPLE__
	semaphore_destroy(mach_task_self(), *((winpr_sem_t*)semaphore->sem));
#else
	sem_destroy((winpr_sem_t*)semaphore->sem);
#endif
#endif
	free(semaphore);
	return TRUE;
}

static HANDLE_OPS ops = { SemaphoreIsHandled,
	                      SemaphoreCloseHandle,
	                      SemaphoreGetFd,
	                      SemaphoreCleanupHandle,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL,
	                      NULL };

HANDLE CreateSemaphoreW(WINPR_ATTR_UNUSED LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
                        LONG lInitialCount, WINPR_ATTR_UNUSED LONG lMaximumCount,
                        WINPR_ATTR_UNUSED LPCWSTR lpName)
{
	HANDLE handle = NULL;
	WINPR_SEMAPHORE* semaphore = NULL;
	semaphore = (WINPR_SEMAPHORE*)calloc(1, sizeof(WINPR_SEMAPHORE));

	if (!semaphore)
		return NULL;

	semaphore->pipe_fd[0] = -1;
	semaphore->pipe_fd[1] = -1;
	semaphore->sem = (winpr_sem_t*)NULL;
	semaphore->common.ops = &ops;
#ifdef WINPR_PIPE_SEMAPHORE

	if (pipe(semaphore->pipe_fd) < 0)
	{
		WLog_ERR(TAG, "failed to create semaphore");
		free(semaphore);
		return NULL;
	}

	while (lInitialCount > 0)
	{
		if (write(semaphore->pipe_fd[1], "-", 1) != 1)
		{
			close(semaphore->pipe_fd[0]);
			close(semaphore->pipe_fd[1]);
			free(semaphore);
			return NULL;
		}

		lInitialCount--;
	}

#else
	semaphore->sem = (winpr_sem_t*)malloc(sizeof(winpr_sem_t));

	if (!semaphore->sem)
	{
		WLog_ERR(TAG, "failed to allocate semaphore memory");
		free(semaphore);
		return NULL;
	}

#if defined __APPLE__

	if (semaphore_create(mach_task_self(), semaphore->sem, SYNC_POLICY_FIFO, lMaximumCount) !=
	    KERN_SUCCESS)
#else
	if (sem_init(semaphore->sem, 0, lMaximumCount) == -1)
#endif
	{
		WLog_ERR(TAG, "failed to create semaphore");
		free(semaphore->sem);
		free(semaphore);
		return NULL;
	}

#endif
	WINPR_HANDLE_SET_TYPE_AND_MODE(semaphore, HANDLE_TYPE_SEMAPHORE, WINPR_FD_READ);
	handle = (HANDLE)semaphore;
	return handle;
}

HANDLE CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount,
                        LONG lMaximumCount, WINPR_ATTR_UNUSED LPCSTR lpName)
{
	return CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, NULL);
}

HANDLE OpenSemaphoreW(WINPR_ATTR_UNUSED DWORD dwDesiredAccess,
                      WINPR_ATTR_UNUSED BOOL bInheritHandle, WINPR_ATTR_UNUSED LPCWSTR lpName)
{
	WLog_ERR(TAG, "not implemented");
	return NULL;
}

HANDLE OpenSemaphoreA(WINPR_ATTR_UNUSED DWORD dwDesiredAccess,
                      WINPR_ATTR_UNUSED BOOL bInheritHandle, WINPR_ATTR_UNUSED LPCSTR lpName)
{
	WLog_ERR(TAG, "not implemented");
	return NULL;
}

BOOL ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount,
                      WINPR_ATTR_UNUSED LPLONG lpPreviousCount)
{
	ULONG Type = 0;
	WINPR_HANDLE* Object = NULL;
	WINPR_SEMAPHORE* semaphore = NULL;

	if (!winpr_Handle_GetInfo(hSemaphore, &Type, &Object))
		return FALSE;

	if (Type == HANDLE_TYPE_SEMAPHORE)
	{
		semaphore = (WINPR_SEMAPHORE*)Object;
#ifdef WINPR_PIPE_SEMAPHORE

		if (semaphore->pipe_fd[0] != -1)
		{
			while (lReleaseCount > 0)
			{
				if (write(semaphore->pipe_fd[1], "-", 1) != 1)
					return FALSE;

				lReleaseCount--;
			}
		}

#else

		while (lReleaseCount > 0)
		{
#if defined __APPLE__
			semaphore_signal(*((winpr_sem_t*)semaphore->sem));
#else
			sem_post((winpr_sem_t*)semaphore->sem);
#endif
		}

#endif
		return TRUE;
	}

	WLog_ERR(TAG, "called on a handle that is not a semaphore");
	return FALSE;
}

#endif
