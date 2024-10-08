/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Digital Sound Processing
 *
 * Copyright 2010-2011 Vic Lee
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

#ifndef FREERDP_CODEC_DSP_H
#define FREERDP_CODEC_DSP_H

#include <winpr/stream.h>

#include <freerdp/api.h>
#include <freerdp/codec/audio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct S_FREERDP_DSP_CONTEXT FREERDP_DSP_CONTEXT;

	FREERDP_API void freerdp_dsp_context_free(FREERDP_DSP_CONTEXT* context);

	WINPR_ATTR_MALLOC(freerdp_dsp_context_free, 1)
	FREERDP_API FREERDP_DSP_CONTEXT* freerdp_dsp_context_new(BOOL encoder);

	FREERDP_API BOOL freerdp_dsp_supports_format(const AUDIO_FORMAT* WINPR_RESTRICT format,
	                                             BOOL encode);
	FREERDP_API BOOL freerdp_dsp_encode(FREERDP_DSP_CONTEXT* WINPR_RESTRICT context,
	                                    const AUDIO_FORMAT* WINPR_RESTRICT srcFormat,
	                                    const BYTE* WINPR_RESTRICT data, size_t length,
	                                    wStream* WINPR_RESTRICT out);
	FREERDP_API BOOL freerdp_dsp_decode(FREERDP_DSP_CONTEXT* WINPR_RESTRICT context,
	                                    const AUDIO_FORMAT* WINPR_RESTRICT srcFormat,
	                                    const BYTE* WINPR_RESTRICT data, size_t length,
	                                    wStream* WINPR_RESTRICT out);

	FREERDP_API BOOL freerdp_dsp_context_reset(FREERDP_DSP_CONTEXT* WINPR_RESTRICT context,
	                                           const AUDIO_FORMAT* WINPR_RESTRICT targetFormat,
	                                           UINT32 FramesPerPacket);

#ifdef __cplusplus
}
#endif

#endif /* FREERDP_CODEC_DSP_H */
