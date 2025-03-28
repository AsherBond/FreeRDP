/* FreeRDP: A Remote Desktop Protocol Client
 * Sign operations.
 * vi:ts=4 sw=4:
 *
 * (c) Copyright 2012 Hewlett-Packard Development Company, L.P.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <freerdp/config.h>

#include <freerdp/types.h>
#include <freerdp/primitives.h>

#include "prim_internal.h"
#include "prim_sign.h"

/* ----------------------------------------------------------------------------
 * Set pDst to the sign-value of the 16-bit values in pSrc (-1, 0, or 1).
 */
static pstatus_t general_sign_16s(const INT16* WINPR_RESTRICT pSrc, INT16* WINPR_RESTRICT pDst,
                                  UINT32 len)
{
	while (len--)
	{
		INT16 src = *pSrc++;
		*pDst++ = WINPR_ASSERTING_INT_CAST(int16_t, (src < 0) ? (-1) : ((src > 0) ? 1 : 0));
	}

	return PRIMITIVES_SUCCESS;
}

/* ------------------------------------------------------------------------- */
void primitives_init_sign(primitives_t* WINPR_RESTRICT prims)
{
	/* Start with the default. */
	prims->sign_16s = general_sign_16s;
}

void primitives_init_sign_opt(primitives_t* WINPR_RESTRICT prims)
{
	primitives_init_sign(prims);
	primitives_init_sign_ssse3(prims);
}
