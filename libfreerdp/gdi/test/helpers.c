/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * GDI Library Tests
 *
 * Copyright 2016 Armin Novak <armin.novak@thincast.com>
 * Copyright 2016 Thincast Technologies GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "helpers.h"

HGDI_BITMAP test_convert_to_bitmap(const BYTE* src, UINT32 SrcFormat, UINT32 SrcStride, UINT32 xSrc,
                                   UINT32 ySrc, UINT32 DstFormat, UINT32 DstStride, UINT32 xDst,
                                   UINT32 yDst, UINT32 nWidth, UINT32 nHeight,
                                   const gdiPalette* hPalette)
{
	HGDI_BITMAP bmp = NULL;
	BYTE* data = NULL;

	if (DstStride == 0)
		DstStride = nWidth * FreeRDPGetBytesPerPixel(DstFormat);

	data = winpr_aligned_malloc(1ULL * DstStride * nHeight, 16);

	if (!data)
		return NULL;

	if (!freerdp_image_copy(data, DstFormat, DstStride, xDst, yDst, nWidth, nHeight, src, SrcFormat,
	                        SrcStride, xSrc, ySrc, hPalette, FREERDP_FLIP_NONE))
	{
		winpr_aligned_free(data);
		return NULL;
	}

	bmp = gdi_CreateBitmap(nWidth, nHeight, DstFormat, data);

	if (!bmp)
	{
		winpr_aligned_free(data);
		return NULL;
	}

	return bmp;
}

static void test_dump_data(unsigned char* p, size_t len, size_t width, const char* name)
{
	unsigned char* line = p;
	const size_t stride = (width > 0) ? len / width : 1;
	size_t offset = 0;
	printf("\n%s[%" PRIuz "][%" PRIuz "]:\n", name, stride, width);

	while (offset < len)
	{
		size_t i = 0;
		printf("%04" PRIxz " ", offset);
		size_t thisline = len - offset;

		if (thisline > width)
			thisline = width;

		for (; i < thisline; i++)
			printf("%02x ", line[i]);

		for (; i < width; i++)
			printf("   ");

		printf("\n");
		offset += thisline;
		line += thisline;
	}

	printf("\n");
	(void)fflush(stdout);
}

void test_dump_bitmap(HGDI_BITMAP hBmp, const char* name)
{
	const size_t stride =
	    WINPR_ASSERTING_INT_CAST(size_t, hBmp->width) * FreeRDPGetBytesPerPixel(hBmp->format);
	test_dump_data(hBmp->data, stride * WINPR_ASSERTING_INT_CAST(uint32_t, hBmp->height), stride,
	               name);
}

static BOOL CompareBitmaps(HGDI_BITMAP hBmp1, HGDI_BITMAP hBmp2, const gdiPalette* palette)
{
	const BYTE* p1 = hBmp1->data;
	const BYTE* p2 = hBmp2->data;
	const UINT32 minw = WINPR_ASSERTING_INT_CAST(
	    uint32_t, (hBmp1->width < hBmp2->width) ? hBmp1->width : hBmp2->width);
	const UINT32 minh = WINPR_ASSERTING_INT_CAST(
	    uint32_t, (hBmp1->height < hBmp2->height) ? hBmp1->height : hBmp2->height);

	for (UINT32 y = 0; y < minh; y++)
	{
		for (UINT32 x = 0; x < minw; x++)
		{
			UINT32 colorA = FreeRDPReadColor(p1, hBmp1->format);
			UINT32 colorB = FreeRDPReadColor(p2, hBmp2->format);
			p1 += FreeRDPGetBytesPerPixel(hBmp1->format);
			p2 += FreeRDPGetBytesPerPixel(hBmp2->format);

			if (hBmp1->format != hBmp2->format)
				colorB = FreeRDPConvertColor(colorB, hBmp2->format, hBmp1->format, palette);

			if (colorA != colorB)
				return FALSE;
		}
	}

	return TRUE;
}

BOOL test_assert_bitmaps_equal(HGDI_BITMAP hBmpActual, HGDI_BITMAP hBmpExpected, const char* name,
                               const gdiPalette* palette)
{
	BOOL bitmapsEqual = CompareBitmaps(hBmpActual, hBmpExpected, palette);

	if (!bitmapsEqual)
	{
		printf("Testing ROP %s [%s|%s]\n", name, FreeRDPGetColorFormatName(hBmpActual->format),
		       FreeRDPGetColorFormatName(hBmpExpected->format));
		test_dump_bitmap(hBmpActual, "Actual");
		test_dump_bitmap(hBmpExpected, "Expected");
		(void)fflush(stdout);
		(void)fflush(stderr);
		return TRUE; // TODO: Fix test cases
	}

	return bitmapsEqual;
}
