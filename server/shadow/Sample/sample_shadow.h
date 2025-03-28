/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 *
 * Copyright 2025 Armin Novak <anovak@thincast.com>
 * Copyright 2025 Thincast Technologies GmbH
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

#pragma once

#include <freerdp/server/shadow.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct sample_shadow_subsystem sampleShadowSubsystem;

	struct sample_shadow_subsystem
	{
		rdpShadowSubsystem base;

		/* Additional platform specific stuff goes here */
	};

	FREERDP_API const char* ShadowSubsystemName(void);
	FREERDP_API int ShadowSubsystemEntry(RDP_SHADOW_ENTRY_POINTS* pEntryPoints);

#ifdef __cplusplus
}
#endif
