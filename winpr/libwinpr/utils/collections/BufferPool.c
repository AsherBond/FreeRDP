/**
 * WinPR: Windows Portable Runtime
 * Buffer Pool
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

#include <winpr/crt.h>

#include <winpr/collections.h>

#define MAX(a, b) ((a) > (b)) ? (a) : (b)

typedef struct
{
	SSIZE_T size;
	void* buffer;
} wBufferPoolItem;

struct s_wBufferPool
{
	SSIZE_T fixedSize;
	DWORD alignment;
	BOOL synchronized;
	CRITICAL_SECTION lock;

	SSIZE_T size;
	SSIZE_T capacity;
	void** array;

	SSIZE_T aSize;
	SSIZE_T aCapacity;
	wBufferPoolItem* aArray;

	SSIZE_T uSize;
	SSIZE_T uCapacity;
	wBufferPoolItem* uArray;
};

static BOOL BufferPool_Lock(wBufferPool* pool)
{
	if (!pool)
		return FALSE;

	if (pool->synchronized)
		EnterCriticalSection(&pool->lock);
	return TRUE;
}

static BOOL BufferPool_Unlock(wBufferPool* pool)
{
	if (!pool)
		return FALSE;

	if (pool->synchronized)
		LeaveCriticalSection(&pool->lock);
	return TRUE;
}

/**
 * C equivalent of the C# BufferManager Class:
 * http://msdn.microsoft.com/en-us/library/ms405814.aspx
 */

/**
 * Methods
 */

static BOOL BufferPool_ShiftAvailable(wBufferPool* pool, size_t index, int count)
{
	if (count > 0)
	{
		if (pool->aSize + count > pool->aCapacity)
		{
			wBufferPoolItem* newArray = NULL;
			SSIZE_T newCapacity = pool->aSize + count;
			newCapacity += (newCapacity + 2) / 2;

			WINPR_ASSERT(newCapacity > 0);
			if (pool->alignment > 0)
				newArray = (wBufferPoolItem*)winpr_aligned_realloc(
				    pool->aArray,
				    sizeof(wBufferPoolItem) * WINPR_ASSERTING_INT_CAST(size_t, newCapacity),
				    pool->alignment);
			else
				newArray = (wBufferPoolItem*)realloc(
				    pool->aArray,
				    sizeof(wBufferPoolItem) * WINPR_ASSERTING_INT_CAST(size_t, newCapacity));
			if (!newArray)
				return FALSE;
			pool->aArray = newArray;
			pool->aCapacity = newCapacity;
		}

		MoveMemory(
		    &pool->aArray[index + WINPR_ASSERTING_INT_CAST(size_t, count)], &pool->aArray[index],
		    (WINPR_ASSERTING_INT_CAST(size_t, pool->aSize) - index) * sizeof(wBufferPoolItem));
		pool->aSize += count;
	}
	else if (count < 0)
	{
		MoveMemory(
		    &pool->aArray[index], &pool->aArray[index + WINPR_ASSERTING_INT_CAST(size_t, -count)],
		    (WINPR_ASSERTING_INT_CAST(size_t, pool->aSize) - index) * sizeof(wBufferPoolItem));
		pool->aSize += count;
	}
	return TRUE;
}

static BOOL BufferPool_ShiftUsed(wBufferPool* pool, SSIZE_T index, SSIZE_T count)
{
	if (count > 0)
	{
		if (pool->uSize + count > pool->uCapacity)
		{
			SSIZE_T newUCapacity = pool->uCapacity * 2;
			wBufferPoolItem* newUArray = NULL;
			if (pool->alignment > 0)
				newUArray = (wBufferPoolItem*)winpr_aligned_realloc(
				    pool->uArray,
				    sizeof(wBufferPoolItem) * WINPR_ASSERTING_INT_CAST(size_t, newUCapacity),
				    pool->alignment);
			else
				newUArray = (wBufferPoolItem*)realloc(
				    pool->uArray,
				    sizeof(wBufferPoolItem) * WINPR_ASSERTING_INT_CAST(size_t, newUCapacity));
			if (!newUArray)
				return FALSE;
			pool->uCapacity = newUCapacity;
			pool->uArray = newUArray;
		}

		MoveMemory(&pool->uArray[index + count], &pool->uArray[index],
		           WINPR_ASSERTING_INT_CAST(size_t, pool->uSize - index) * sizeof(wBufferPoolItem));
		pool->uSize += count;
	}
	else if (count < 0)
	{
		MoveMemory(&pool->uArray[index], &pool->uArray[index - count],
		           WINPR_ASSERTING_INT_CAST(size_t, pool->uSize - index) * sizeof(wBufferPoolItem));
		pool->uSize += count;
	}
	return TRUE;
}

/**
 * Get the buffer pool size
 */

SSIZE_T BufferPool_GetPoolSize(wBufferPool* pool)
{
	SSIZE_T size = 0;

	BufferPool_Lock(pool);

	if (pool->fixedSize)
	{
		/* fixed size buffers */
		size = pool->size;
	}
	else
	{
		/* variable size buffers */
		size = pool->uSize;
	}

	BufferPool_Unlock(pool);

	return size;
}

/**
 * Get the size of a pooled buffer
 */

SSIZE_T BufferPool_GetBufferSize(wBufferPool* pool, const void* buffer)
{
	SSIZE_T size = 0;
	BOOL found = FALSE;

	BufferPool_Lock(pool);

	if (pool->fixedSize)
	{
		/* fixed size buffers */
		size = pool->fixedSize;
		found = TRUE;
	}
	else
	{
		/* variable size buffers */

		for (SSIZE_T index = 0; index < pool->uSize; index++)
		{
			if (pool->uArray[index].buffer == buffer)
			{
				size = pool->uArray[index].size;
				found = TRUE;
				break;
			}
		}
	}

	BufferPool_Unlock(pool);

	return (found) ? size : -1;
}

/**
 * Gets a buffer of at least the specified size from the pool.
 */

void* BufferPool_Take(wBufferPool* pool, SSIZE_T size)
{
	SSIZE_T maxSize = 0;
	SSIZE_T maxIndex = 0;
	SSIZE_T foundIndex = -1;
	BOOL found = FALSE;
	void* buffer = NULL;

	BufferPool_Lock(pool);

	if (pool->fixedSize)
	{
		/* fixed size buffers */

		if (pool->size > 0)
			buffer = pool->array[--(pool->size)];

		if (!buffer)
		{
			if (pool->alignment)
				buffer = winpr_aligned_malloc(WINPR_ASSERTING_INT_CAST(size_t, pool->fixedSize),
				                              pool->alignment);
			else
				buffer = malloc(WINPR_ASSERTING_INT_CAST(size_t, pool->fixedSize));
		}

		if (!buffer)
			goto out_error;
	}
	else
	{
		/* variable size buffers */

		maxSize = 0;
		maxIndex = 0;

		if (size < 1)
			size = pool->fixedSize;

		for (SSIZE_T index = 0; index < pool->aSize; index++)
		{
			if (pool->aArray[index].size > maxSize)
			{
				maxIndex = index;
				maxSize = pool->aArray[index].size;
			}

			if (pool->aArray[index].size >= size)
			{
				foundIndex = index;
				found = TRUE;
				break;
			}
		}

		if (!found && maxSize)
		{
			foundIndex = maxIndex;
			found = TRUE;
		}

		if (!found)
		{
			if (!size)
				buffer = NULL;
			else
			{
				if (pool->alignment)
					buffer = winpr_aligned_malloc(WINPR_ASSERTING_INT_CAST(size_t, size),
					                              pool->alignment);
				else
					buffer = malloc(WINPR_ASSERTING_INT_CAST(size_t, size));

				if (!buffer)
					goto out_error;
			}
		}
		else
		{
			buffer = pool->aArray[foundIndex].buffer;

			if (maxSize < size)
			{
				void* newBuffer = NULL;
				if (pool->alignment)
					newBuffer = winpr_aligned_realloc(
					    buffer, WINPR_ASSERTING_INT_CAST(size_t, size), pool->alignment);
				else
					newBuffer = realloc(buffer, WINPR_ASSERTING_INT_CAST(size_t, size));

				if (!newBuffer)
					goto out_error_no_free;

				buffer = newBuffer;
			}

			if (!BufferPool_ShiftAvailable(pool, WINPR_ASSERTING_INT_CAST(size_t, foundIndex), -1))
				goto out_error;
		}

		if (!buffer)
			goto out_error;

		if (pool->uSize + 1 > pool->uCapacity)
		{
			size_t newUCapacity = WINPR_ASSERTING_INT_CAST(size_t, pool->uCapacity);
			newUCapacity += (newUCapacity + 2) / 2;
			if (newUCapacity > SSIZE_MAX)
				goto out_error;
			wBufferPoolItem* newUArray =
			    (wBufferPoolItem*)realloc(pool->uArray, sizeof(wBufferPoolItem) * newUCapacity);
			if (!newUArray)
				goto out_error;

			pool->uCapacity = (SSIZE_T)newUCapacity;
			pool->uArray = newUArray;
		}

		pool->uArray[pool->uSize].buffer = buffer;
		pool->uArray[pool->uSize].size = size;
		(pool->uSize)++;
	}

	BufferPool_Unlock(pool);

	return buffer;

out_error:
	if (pool->alignment)
		winpr_aligned_free(buffer);
	else
		free(buffer);
out_error_no_free:
	BufferPool_Unlock(pool);
	return NULL;
}

/**
 * Returns a buffer to the pool.
 */

BOOL BufferPool_Return(wBufferPool* pool, void* buffer)
{
	BOOL rc = FALSE;
	SSIZE_T size = 0;
	BOOL found = FALSE;

	BufferPool_Lock(pool);

	if (pool->fixedSize)
	{
		/* fixed size buffers */

		if ((pool->size + 1) >= pool->capacity)
		{
			SSIZE_T newCapacity = MAX(2, pool->size + (pool->size + 2) / 2 + 1);
			void** newArray = (void**)realloc(
			    (void*)pool->array, sizeof(void*) * WINPR_ASSERTING_INT_CAST(size_t, newCapacity));
			if (!newArray)
				goto out_error;

			pool->capacity = newCapacity;
			pool->array = newArray;
		}

		pool->array[(pool->size)++] = buffer;
	}
	else
	{
		/* variable size buffers */

		SSIZE_T index = 0;
		for (; index < pool->uSize; index++)
		{
			if (pool->uArray[index].buffer == buffer)
			{
				found = TRUE;
				break;
			}
		}

		if (found)
		{
			size = pool->uArray[index].size;
			if (!BufferPool_ShiftUsed(pool, index, -1))
				goto out_error;
		}

		if (size)
		{
			if ((pool->aSize + 1) >= pool->aCapacity)
			{
				SSIZE_T newCapacity = MAX(2, pool->aSize + (pool->aSize + 2) / 2 + 1);
				wBufferPoolItem* newArray = (wBufferPoolItem*)realloc(
				    pool->aArray,
				    sizeof(wBufferPoolItem) * WINPR_ASSERTING_INT_CAST(size_t, newCapacity));
				if (!newArray)
					goto out_error;

				pool->aCapacity = newCapacity;
				pool->aArray = newArray;
			}

			pool->aArray[pool->aSize].buffer = buffer;
			pool->aArray[pool->aSize].size = size;
			(pool->aSize)++;
		}
	}

	rc = TRUE;
out_error:
	BufferPool_Unlock(pool);
	return rc;
}

/**
 * Releases the buffers currently cached in the pool.
 */

void BufferPool_Clear(wBufferPool* pool)
{
	BufferPool_Lock(pool);

	if (pool->fixedSize)
	{
		/* fixed size buffers */

		while (pool->size > 0)
		{
			(pool->size)--;

			if (pool->alignment)
				winpr_aligned_free(pool->array[pool->size]);
			else
				free(pool->array[pool->size]);
		}
	}
	else
	{
		/* variable size buffers */

		while (pool->aSize > 0)
		{
			(pool->aSize)--;

			if (pool->alignment)
				winpr_aligned_free(pool->aArray[pool->aSize].buffer);
			else
				free(pool->aArray[pool->aSize].buffer);
		}

		while (pool->uSize > 0)
		{
			(pool->uSize)--;

			if (pool->alignment)
				winpr_aligned_free(pool->uArray[pool->uSize].buffer);
			else
				free(pool->uArray[pool->uSize].buffer);
		}
	}

	BufferPool_Unlock(pool);
}

/**
 * Construction, Destruction
 */

wBufferPool* BufferPool_New(BOOL synchronized, SSIZE_T fixedSize, DWORD alignment)
{
	wBufferPool* pool = NULL;

	pool = (wBufferPool*)calloc(1, sizeof(wBufferPool));

	if (pool)
	{
		pool->fixedSize = fixedSize;

		if (pool->fixedSize < 0)
			pool->fixedSize = 0;

		pool->alignment = alignment;
		pool->synchronized = synchronized;

		if (pool->synchronized)
			InitializeCriticalSectionAndSpinCount(&pool->lock, 4000);

		if (pool->fixedSize)
		{
			/* fixed size buffers */

			pool->size = 0;
			pool->capacity = 32;
			pool->array =
			    (void**)calloc(WINPR_ASSERTING_INT_CAST(size_t, pool->capacity), sizeof(void*));
			if (!pool->array)
				goto out_error;
		}
		else
		{
			/* variable size buffers */

			pool->aSize = 0;
			pool->aCapacity = 32;
			pool->aArray = (wBufferPoolItem*)calloc(
			    WINPR_ASSERTING_INT_CAST(size_t, pool->aCapacity), sizeof(wBufferPoolItem));
			if (!pool->aArray)
				goto out_error;

			pool->uSize = 0;
			pool->uCapacity = 32;
			pool->uArray = (wBufferPoolItem*)calloc(
			    WINPR_ASSERTING_INT_CAST(size_t, pool->uCapacity), sizeof(wBufferPoolItem));
			if (!pool->uArray)
				goto out_error;
		}
	}

	return pool;

out_error:
	WINPR_PRAGMA_DIAG_PUSH
	WINPR_PRAGMA_DIAG_IGNORED_MISMATCHED_DEALLOC
	BufferPool_Free(pool);
	WINPR_PRAGMA_DIAG_POP
	return NULL;
}

void BufferPool_Free(wBufferPool* pool)
{
	if (pool)
	{
		BufferPool_Clear(pool);

		if (pool->synchronized)
			DeleteCriticalSection(&pool->lock);

		if (pool->fixedSize)
		{
			/* fixed size buffers */

			free((void*)pool->array);
		}
		else
		{
			/* variable size buffers */

			free(pool->aArray);
			free(pool->uArray);
		}

		free(pool);
	}
}
