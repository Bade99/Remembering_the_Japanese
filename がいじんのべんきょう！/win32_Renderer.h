#pragma once
#include "windows_sdk.h"
#include <windowsx.h>
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "unCap_Math.h"

#define UNCAP_GDIPLUS /*For lazy bilinear filtered drawing*/

#ifdef UNCAP_GDIPLUS
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
#endif

#ifdef _DEBUG
#define COBJMACROS

#include <Objbase.h>
#include <wincodec.h>
#include <Winerror.h>

#pragma comment(lib, "Windowscodecs.lib")

HRESULT WriteBitmap(HBITMAP bitmap, const cstr* pathname) {
	//TODO(fran): solve limitations https://stackoverflow.com/questions/24720451/save-hbitmap-to-bmp-file-using-only-win32
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	HRESULT hr = S_OK;

	// (1) Retrieve properties from the source HBITMAP.
	BITMAP bm_info = { 0 };
	if (!GetObject(bitmap, sizeof(bm_info), &bm_info))
		hr = E_FAIL;

	// (2) Create an IWICImagingFactory instance.
	IWICImagingFactory* factory = NULL;
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory, (LPVOID*)&factory);

	// (3) Create an IWICBitmap instance from the HBITMAP.
	IWICBitmap* wic_bitmap = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateBitmapFromHBITMAP(bitmap, NULL,
			WICBitmapIgnoreAlpha,
			&wic_bitmap);

	// (4) Create an IWICStream instance, and attach it to a filename.
	IWICStream* stream = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateStream(&stream);
	if (SUCCEEDED(hr))
		hr = stream->InitializeFromFilename(pathname, GENERIC_WRITE);

	// (5) Create an IWICBitmapEncoder instance, and associate it with the stream.
	IWICBitmapEncoder* encoder = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateEncoder(GUID_ContainerFormatBmp, NULL,
			&encoder);
	if (SUCCEEDED(hr))
		hr = encoder->Initialize((IStream*)stream,
			WICBitmapEncoderNoCache);

	// (6) Create an IWICBitmapFrameEncode instance, and initialize it
	// in compliance with the source HBITMAP.
	IWICBitmapFrameEncode* frame = NULL;
	if (SUCCEEDED(hr))
		hr = encoder->CreateNewFrame(&frame, NULL);
	if (SUCCEEDED(hr))
		hr = frame->Initialize(NULL);
	if (SUCCEEDED(hr))
		hr = frame->SetSize(bm_info.bmWidth, bm_info.bmHeight);
	if (SUCCEEDED(hr)) {
		GUID pixel_format;
		if (bm_info.bmBitsPixel == 32)		pixel_format = GUID_WICPixelFormat32bppBGRA;
		else if (bm_info.bmBitsPixel == 24)	pixel_format = GUID_WICPixelFormat24bppBGR;
		else if (bm_info.bmBitsPixel == 1)	pixel_format = GUID_WICPixelFormatBlackWhite;
		else Assert(0);

		hr = frame->SetPixelFormat(&pixel_format);
	}

	// (7) Write bitmap data to the frame.
	if (SUCCEEDED(hr))
		hr = frame->WriteSource((IWICBitmapSource*)wic_bitmap,
			NULL);

	// (8) Commit frame and data to stream.
	if (SUCCEEDED(hr))
		hr = frame->Commit();
	if (SUCCEEDED(hr))
		hr = encoder->Commit();

	// Cleanup
	if (frame)
		frame->Release();
	if (encoder)
		encoder->Release();
	if (stream)
		stream->Release();
	if (wic_bitmap)
		wic_bitmap->Release();
	if (factory)
		factory->Release();

	return hr;
	CoUninitialize();
}
#else
HRESULT WriteBitmap(HBITMAP bitmap, const cstr* pathname = L"");
#endif

int FillRectAlpha(HDC dc, RECT* r, HBRUSH br, u8 alpha) {
	COLORREF color = ColorFromBrush(br);
	color |= (alpha << 24);

	BITMAPINFO nfo;
	nfo.bmiHeader.biSize = sizeof(nfo.bmiHeader);
	nfo.bmiHeader.biWidth = 1;
	nfo.bmiHeader.biHeight = -1;
	nfo.bmiHeader.biPlanes = 1;
	nfo.bmiHeader.biBitCount = 32;
	nfo.bmiHeader.biCompression = BI_RGB;
	nfo.bmiHeader.biSizeImage = 0;
	nfo.bmiHeader.biXPelsPerMeter = 0;
	nfo.bmiHeader.biYPelsPerMeter = 0;
	nfo.bmiHeader.biClrUsed = 0;
	nfo.bmiHeader.biClrImportant = 0;

	int res = StretchDIBits(dc, r->left, r->top, RECTWIDTH((*r)), RECTHEIGHT((*r)), 0, 0, 1, 1, &color, &nfo, DIB_RGB_COLORS, SRCCOPY);
	return res;
}

struct img {
	i32 width;
	i32 height;
	i32 pitch;
	u8 bits_per_pixel;
	void* mem;
};

struct mask_bilinear_sample { u8 sample[4]; };
mask_bilinear_sample sample_bilinear_mask(img* mask, i32 x, i32 y) {
	//NOTE: handmade 108 1:20:00 interesting, once you scale an img to half size or more you get sampling errors cause you're no longer sampling all the pixels involved, that's when you need MIPMAPPING

	u8 idx = 7 - (x % 8);

	x /= 8;

	u8* texel_ptr = ((u8*)mask->mem + y * mask->pitch + x * 1);

	u8 tex[4];

	//TODO(fran): remove duplicate pixel reads by applying the "&" afterwards
	tex[0] = (*(u8*)texel_ptr) & (1 << idx); //NOTE: For a better blend we pick the colors around the texel, movement is much smoother
	tex[1] = idx == 0 ? (x < mask->width ? (*(u8*)(texel_ptr + 1)) & (1 << 7) : 0xFF) : (*(u8*)texel_ptr) & (1 << (idx - 1));
	tex[2] = y < mask->height ? (*(u8*)(texel_ptr + mask->pitch)) & (1 << idx) : 0xFF;
	tex[3] = idx == 0 ? (x < mask->width ? (*(u8*)(texel_ptr + mask->pitch + 1)) & (1 << 7) : 0xFF) : (y < mask->height ? (*(u8*)(texel_ptr + mask->pitch)) & (1 << (idx - 1)) : 0xFF);

	mask_bilinear_sample sample;
	for (int i = 0; i < 4; i++)
		sample.sample[i] = tex[i];

	return sample;
}

//---------------Ring rendering---------------:

f32 signum(f32 val) { return (f32)((f32(0) < val) - (val < f32(0))); }

f32 isGreaterEqThanZero(f32 num) { return signum(signum(num) + 1); }

f32 signOf(f32 num) { return signum(num) + !num; }

f32 modulus(f32 num, f32 base) { return (f32)fmod(fmod(num, base) + base, base); }

f32 isLowerEqThanZero(f32 num) { return isGreaterEqThanZero(-num); }

f32 isGreaterEqThan(f32 a, f32 b) { return isGreaterEqThanZero(a - b); }

f32 isLowerEqThan(f32 a, f32 b) { return isLowerEqThanZero(a - b); }

f32 isGreaterThanZero(f32 num) { return !isLowerEqThanZero(num); }

f32 saturate(f32 n) { return clamp01(n); }

constexpr f32 PI = 3.1415f;
constexpr f32 _2PI = PI*2.f;
constexpr f32 stripeRadialStart = 0.30f;
constexpr f32 stripeRadialEnd = 0.48f;
constexpr f32 stripeDist = stripeRadialEnd - stripeRadialStart;

f32 edgeTransparency(v2 absolutePos, v2 circleOrigin, float circleRadius)
{
	v2 center = v2{ 0.5f, 0.5f };
	f32 distFromCircleOrigin = length(absolutePos - circleOrigin);

	v2 pos = absolutePos - center;
	f32 radialDist = length(pos);

	f32 transparency = powf(cosf(distFromCircleOrigin * (PI / stripeDist)) + 0.8f, 10) * isLowerEqThan(distFromCircleOrigin, circleRadius);

	return transparency;
}

v4 main_ring(v2 uv, f32 completion, v3 color) //2nd pass
{
	//Shader taken from https://github.com/ankomas/2019_1C_K3051_BackBuffer/blob/master/TGC.Group/Shaders/Oxygen.fx
	//TODO(fran): all the extra functions this guy added are probably pointless and should be taken out, probably there's some extra perf waiting for us there
	v2 absolutePos = uv;
	v2 center = v2{ 0.5f, 0.5f };
	v2 pos = absolutePos - center;
	f32 radialDist = length(pos);

	//f32 angleForColor = (PI * isGreaterEqThanZero(pos.y) - signOf(pos.y) * asinf(abs(pos.x) / radialDist));
	f32 primitiveAngle = (PI * isGreaterEqThanZero(pos.y) - signOf(pos.y) * asinf(abs(pos.x) / radialDist));
	f32 realAngle = modulus(primitiveAngle - isLowerEqThanZero(pos.x) * 2.f * primitiveAngle, _2PI);
	//f32 blueIntensity = angleForColor / PI;

	f32 transparency =
		isGreaterEqThan(radialDist, stripeRadialStart)
		* isLowerEqThan(radialDist, stripeRadialEnd)
		* isLowerEqThan(realAngle, completion * _2PI)
		* powf(sinf((radialDist - stripeRadialStart) * (PI / stripeDist)) + 0.8f, 10);

	f32 circleDistanceFromOrigin = stripeRadialStart + stripeDist / 2;
	v2 circleOrigin = v2{ 0.5f, 0.5f - circleDistanceFromOrigin };
	v2 o2AngleUnitVector = v2{ sinf(completion * _2PI), -cosf(completion * _2PI) };
	v2 secondCircleOrigin = center + o2AngleUnitVector * circleDistanceFromOrigin;

	f32 circleRadius = stripeDist / 2;

	f32 lowCircleTransparency = edgeTransparency(absolutePos, circleOrigin, circleRadius);
	f32 highCircleTransparency = edgeTransparency(absolutePos, secondCircleOrigin, circleRadius);

	float finalTransparency = max(transparency, max(lowCircleTransparency, highCircleTransparency));

	//return v4{ 1.f - completion, completion, blueIntensity, completion * finalTransparency };
	v4 res; 
	res.rgb = color;
	res.a = completion * finalTransparency;
	return res;
}

v4 srgb255_to_linear1(v4 v) {
	//Apply 2.2, aka 2, gamma correction
	v4 res;
	//NOTE: we assume alpha to be in linear space, probably it is
	f32 inv255 = 1.f / 255.f;
	res.r = squared(v.r * inv255);
	res.g = squared(v.g * inv255);
	res.b = squared(v.b * inv255);
	res.a = v.a * inv255;
	return res;
}

v4 linear1_to_srgb255(v4 v) {
	v4 res;
	//NOTE: we assume alpha to be in linear space, probably it is
	res.r = square_root(v.r) * 255.f;
	res.g = square_root(v.g) * 255.f;
	res.b = square_root(v.b) * 255.f;
	res.a = v.a * 255.f;
	return res;
}

v4 COLORREF_to_v4_linear1(COLORREF col, u8 alpha) {
	//NOTE: we separate alpha since colorref doesnt really care or set alpha usually
	v4 res;
	res.r = (f32)GetRValue(col);
	res.g = (f32)GetGValue(col);
	res.b = (f32)GetBValue(col);
	res.a = (f32)alpha;

	res = srgb255_to_linear1(res);

	return res;
};

#define RGBA(r,g,b,a)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))

#define GetAValue(rgb)      (LOBYTE((rgb)>>24))

COLORREF v4_linear1_to_COLORREF(v4 col) {
	//NOTE: no idea what to do with alpha since colorref doesnt really care or set alpha usually
	v4 col255 = linear1_to_srgb255(col);
	COLORREF res = RGBA((u32)col255.r, (u32)col255.g, (u32)col255.b, (u32)col255.a);
	return res;
};

f32 makeCircle(v2 absolutePos, v2 center, f32 radius)
{
	f32 radialDist = length(absolutePos - center);
	f32 transparency = powf(.95f + cosf(radialDist * (PI / (radius * 2))), 30);

	return isLowerEqThan(radialDist, radius) * saturate(transparency);
}

v4 main_outter_circle(v2 uv, v3 ring_bk_color) //1st pass
{
	f32 saturation = makeCircle(uv, v2{ .5f, .5f }, .5f);
#if 0
	return v4{ isLowerEqThanZero(completion) * .3f, 0, 0, saturation * .8f };
	//TODO(fran): I believe the idea for the red component is that if completion gets to zero it shows the bk in a red-ish color, we could keep that
#else
	v4 res;
	res.rgb = ring_bk_color;
	res.a = saturation * .8f;
	return res;
#endif
}

v4 main_inner_circle(v2 uv, v3 inner_circle_color) //3rd pass
{
	v2 position = uv;
	v2 center = v2{ .5f, .5f };
	f32 radius = .29f;
	f32 saturation = makeCircle(position, center, radius);
#if 0
	v3 color = v3{ 0.1 + isLowerEqThanZero(oxygen) * 0.3 * 0.5, 0.2, 0.2 };
	//TODO(fran): same idea as in main_outter_circle here
	float light = 1 - (position.y - center.x - radius) / (radius * 2);
#else
	v3 color = inner_circle_color;
	float light = 1;//NOTE: we want a flat look
#endif

	v4 res;
	res.rgb = color * light;
	res.a = saturation;
	return res;
}
//-------------------------------------------:


namespace urender {

	//TODO(fran): use something with alpha (png?) for rendering masks https://stackoverflow.com/questions/1505586/gdi-using-drawimage-to-draw-a-transperancy-mask-of-the-source-image


	//NOTE: the caller is in charge of handling the hbitmap, including deletion
	//NOTE: we add an extra limitation, no offsets, we scale the entire bitmap, at least for now to make it simpler to write
	HBITMAP scale_mask(HBITMAP mask_bmp, int destW, int destH) {

		BITMAP masknfo; GetObject(mask_bmp, sizeof(masknfo), &masknfo); Assert(masknfo.bmBitsPixel == 1);
		img mask;
		mask.width = masknfo.bmWidth;
		mask.height = masknfo.bmHeight;
		mask.bits_per_pixel = 1;
		mask.pitch = masknfo.bmWidthBytes;
		int mask_sz = mask.height * mask.pitch;
		mask.mem = malloc(mask_sz); defer{ free(mask.mem); };

		int getbits = GetBitmapBits(mask_bmp, mask_sz, mask.mem);
		Assert(getbits == mask_sz);

		img scaled_mask;
		scaled_mask.width = destW;
		scaled_mask.height = destH;
		scaled_mask.bits_per_pixel = 1;
		scaled_mask.pitch = round2up((i32)roundf(((f32)scaled_mask.width * (1.f / 8.f/*bytes per pixel*/))));//NOTE: windows expects word aligned bmps
		int scaled_mask_sz = scaled_mask.height * scaled_mask.pitch;
		scaled_mask.mem = (u8*)malloc(scaled_mask_sz); defer{ free(scaled_mask.mem); };


		v2 origin{ 0,0 };
		v2 x_axis{ (f32)scaled_mask.width,0 };
		v2 y_axis{ 0,(f32)scaled_mask.height };

		i32 width_max = scaled_mask.width - 1;
		i32 height_max = scaled_mask.height - 1;

		i32 x_min = scaled_mask.width;
		i32 x_max = 0;
		i32 y_min = scaled_mask.height;
		i32 y_max = 0;

		v2 points[4] = { origin,origin + x_axis,origin + y_axis,origin + x_axis + y_axis };
		for (v2 p : points) {
			i32 floorx = (i32)floorf(p.x);
			i32 ceilx = (i32)ceilf(p.x);
			i32 floory = (i32)floorf(p.y);
			i32 ceily = (i32)ceilf(p.y);

			if (x_min > floorx)x_min = floorx;
			if (y_min > floory)y_min = floory;
			if (x_max < ceilx)x_max = ceilx;
			if (y_max < ceily)y_max = ceily;
		}

		if (x_min < 0)x_min = 0;
		if (y_min < 0)y_min = 0;
		if (x_max > scaled_mask.width) x_max = scaled_mask.width;
		if (y_max > scaled_mask.height)y_max = scaled_mask.height;

		u8* row = (u8*)scaled_mask.mem /*+(x_min *(i32)((f32)scaled_mask.bits_per_pixel/8.f))*/ + y_min * scaled_mask.pitch;

		for (int y = y_min; y < y_max; y++) {//n bits high
			u8* pixel = row;
			u8 index = 7;
			u8 pixels_to_write = 0;
			for (int x = x_min; x < x_max; x++) {//n bits wide

				f32 u = (f32)x / (f32)(scaled_mask.width - 1);
				f32 v = (f32)y / (f32)(scaled_mask.height - 1);

				f32 t_x = u * (f32)(mask.width - 1);
				f32 t_y = v * (f32)(mask.height - 1);

				i32 i_x = (i32)t_x;
				i32 i_y = (i32)t_y;

				f32 f_x = t_x - (f32)i_x;
				f32 f_y = t_y - (f32)i_y;

				Assert(i_x >= 0 && i_x < mask.width);
				Assert(i_y >= 0 && i_y < mask.height);

				//Bilinear filtering
				mask_bilinear_sample sample = sample_bilinear_mask(&mask, i_x, i_y);
				v4 f_sample;
				//TODO(fran): lerp and f_x f_y
				for (int i = 0; i < 4; i++) {
					f_sample.comp[i] = (sample.sample[i] ? 1.f : 0.f);
				}

				u8 texel = (u8)roundf(lerp(lerp(f_sample.x, f_x, f_sample.y), f_y, lerp(f_sample.z, f_x, f_sample.w)));

				pixels_to_write |= (texel << index);

				index--;
				if (index == (u8)-1 || (x + 1) == x_max) {
					index = 7;
					//pixels_to_write = ~pixels_to_write;
					*pixel++ = pixels_to_write;
					pixels_to_write = 0;
				}
			}
			row += scaled_mask.pitch;
		}

		//Create bitmap
		HBITMAP new_mask = CreateBitmap(scaled_mask.width, scaled_mask.height, 1, scaled_mask.bits_per_pixel, scaled_mask.mem);
		//TODO(fran): do I need to set the palette?
		return new_mask;
	}

	void draw_mask(HDC dest, int xDest, int yDest, int wDest, int hDest, HBITMAP mask, int xSrc, int ySrc, int wSrc, int hSrc, HBRUSH colorbr) {
		{BITMAP bmpnfo; GetObject(mask, sizeof(bmpnfo), &bmpnfo); Assert(bmpnfo.bmBitsPixel == 1); }

		//Mask Stretching Setup
#if 0 //You can never trust Gdi's stretching functions, they are simply terrible and unusable
		HDC src = CreateCompatibleDC(dest); defer{ DeleteDC(src); };
		//NOTE: src has a 1x1 _monochrome_ bitmap, we wanna be monochrome too
		HBITMAP srcbmp = CreateCompatibleBitmap(src, wDest, hDest); defer{ DeleteObject(srcbmp); };

		HBITMAP oldbmp = (HBITMAP)SelectObject(src, (HGDIOBJ)srcbmp); defer{ SelectObject(src, (HGDIOBJ)oldbmp); };

		HDC maskdc = CreateCompatibleDC(dest); defer{ DeleteDC(maskdc); };

		HBITMAP oldmaskdcbmp = (HBITMAP)SelectObject(maskdc, (HGDIOBJ)mask); defer{ SelectObject(maskdc, (HGDIOBJ)oldmaskdcbmp); };

		//Finally we can actually do the stretching
		int stretchres = StretchBlt(src, 0, 0, wDest, hDest, maskdc, xSrc, ySrc, wSrc, hSrc, SRCCOPY);

		HBRUSH oldbr = (HBRUSH)SelectObject(dest, colorbr); defer{ SelectObject(dest, (HGDIOBJ)oldbr); };

		//TODO(fran): I have no idea why I need to pass the "srcDC", no information from it is needed, and the function explicitly says it should be NULL, but if I do it returns false aka error (some error, cause it also doesnt tell you what it is)
		//NOTE: info on creating your own raster ops https://docs.microsoft.com/en-us/windows/win32/gdi/ternary-raster-operations?redirectedfrom=MSDN
		//Thanks https://stackoverflow.com/questions/778666/raster-operator-to-use-for-maskblt
		HBITMAP stretched_mask = (HBITMAP)GetCurrentObject(src, OBJ_BITMAP);//NOTE: we dont need to do this, we got the hbitmap from before
		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, stretched_mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
#elif 0
		//TODO(fran): https://devblogs.microsoft.com/oldnewthing/20061114-01/?p=29013 raymond always to the rescue, why dont I bitblt to color first and then stretch with gdi+?
		HDC stretch32 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch32); };
		HBITMAP bmp32 = CreateBitmap(wDest, hDest, 1, 32, nullptr); defer{ DeleteObject(bmp32); };
		HBITMAP oldbmp32 = (HBITMAP)SelectObject(stretch32, (HGDIOBJ)bmp32); defer{ SelectObject(stretch32, (HGDIOBJ)oldbmp32); };
		{
			Gdiplus::Graphics graphics(stretch32);//Yeah, who cares, icons are small, I just want bilinear filtering
			graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);

			Gdiplus::Bitmap bmp(mask, 0);//NOTE: gdi converts bitmaps to 32bpp, so, once again, it is useless. Moreover you can ONLY draw to 32bpp dcs with this bmp
			graphics.DrawImage(&bmp,
				Gdiplus::Rect(0, 0, wDest, hDest),
				xSrc, ySrc, wSrc, hSrc,
				Gdiplus::Unit::UnitPixel);//TODO(fran): get the current unit from the device
		}

		HDC stretch24 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch24); };
		HBITMAP bmp24 = CreateBitmap(wDest, hDest, 1, 24, nullptr); defer{ DeleteObject(bmp24); };
		HBITMAP oldbmp24 = (HBITMAP)SelectObject(stretch24, (HGDIOBJ)bmp24); defer{ SelectObject(stretch24, (HGDIOBJ)oldbmp24); };
		BitBlt(stretch24, 0, 0, wDest, hDest, stretch32, 0, 0, SRCCOPY);//32bit to 24bit //THIS DOESNT WOOOOOOOOOOOOOOOOOOOOOOOOOOOORRKKKKKK

		HDC stretch1 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch1); };
		HBITMAP bmp1 = CreateBitmap(wDest, hDest, 1, 1, nullptr); defer{ DeleteObject(bmp1); };
		HBITMAP oldbmp1 = (HBITMAP)SelectObject(stretch1, (HGDIOBJ)bmp1); defer{ SelectObject(stretch1, (HGDIOBJ)oldbmp1); };

		SetBkColor(stretch24, RGB(0xff, 0xff, 0xff));
		SetBkColor(stretch1, RGB(0x00, 0x00, 0x00));
		BitBlt(stretch1, 0, 0, wDest, hDest, stretch24, 0, 0, SRCCOPY);//24bit to 1bit

		BITMAP nfo; GetObject(bmp1, sizeof(nfo), &nfo);
		char bits[128 * 128];
		int r = GetBitmapBits(bmp1, nfo.bmHeight * nfo.bmWidthBytes, bits);

		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, bmp1, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
#else
		HBITMAP scaled_mask = (wDest != wSrc || hDest != hSrc) ? scale_mask(mask, wDest, hDest) : mask; //NOTE: beware of applying any modification to scaled_mask, you could be modifying the original mask
		HBRUSH oldbr = (HBRUSH)SelectObject(dest, colorbr); defer{ SelectObject(dest, (HGDIOBJ)oldbr); };
		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, scaled_mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
		if (scaled_mask != mask)DeleteObject(scaled_mask);
#endif
		Assert(maskres);
	}

	void draw_icon(HDC dest, int xDest, int yDest, int wDest, int hDest, HICON icon, int xSrc, int ySrc, int wSrc, int hSrc) {
		Gdiplus::Graphics graphics(dest);//Yeah, who cares, icons are small, I just want bilinear filtering
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		Gdiplus::Bitmap bmp(icon);
		//TODO?(fran): with gdi+ we can do subpixel placement (RectF)
		graphics.DrawImage(&bmp,
			Gdiplus::Rect(xDest, yDest, wDest, hDest),
			xSrc, ySrc, wSrc, hSrc,
			Gdiplus::Unit::UnitPixel);//TODO(fran): get the current unit from the device
	}

	void draw_bitmap(HDC dest, int xDest, int yDest, int wDest, int hDest, HBITMAP bitmap, int xSrc, int ySrc, int wSrc, int hSrc) {
		Gdiplus::Graphics graphics(dest);//Yeah, who cares, icons are small, I just want bilinear filtering
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		Gdiplus::Bitmap bmp(bitmap,nullptr);//TODO(fran): documentation says it ignores alpha, if that's true then throw this away
		//TODO?(fran): with gdi+ we can do subpixel placement (RectF)
		graphics.DrawImage(&bmp,
			Gdiplus::Rect(xDest, yDest, wDest, hDest),
			xSrc, ySrc, wSrc, hSrc,
			Gdiplus::Unit::UnitPixel);//TODO(fran): get the current unit from the device
	}

	//NOTE: specific procedure for drawing images on windows' menus, other drawing functions may fail on specific situations
	//NOTE: this is set up to work with masks that have black as the color and white as transparency, otherwise you'll need to flip your colors, for example BitBlt(arrowDC, 0, 0, arrowW, arrowH, arrowDC, 0, 0, DSTINVERT)
	void draw_menu_mask(HDC destDC, int xDest, int yDest, int wDest, int hDest, HBITMAP mask, int xSrc, int ySrc, int wSrc, int hSrc, HBRUSH colorbr)
	{
		{BITMAP bmpnfo; GetObject(mask, sizeof(bmpnfo), &bmpnfo); Assert(bmpnfo.bmBitsPixel == 1); }

		//NOTE: I dont know if this will be final, dont really wanna depend on windows, but it's pretty good for now. see https://docs.microsoft.com/en-us/windows/win32/gdi/scaling-an-image maybe some of those stretch modes work better than HALFTONE

		//Create the DCs and Bitmaps we will need
		HDC maskDC = CreateCompatibleDC(destDC);
		HBITMAP fillBitmap = CreateCompatibleBitmap(destDC, wDest, hDest);
		HBITMAP oldFillBitmap = (HBITMAP)SelectObject(maskDC, fillBitmap);

		//Blit the content already stored in dest onto a secondary buffer
		BitBlt(maskDC, 0, 0, wDest, hDest, destDC, xDest, yDest, SRCCOPY);

		//Draw on the new buffer, this is neeeded cause draw_mask uses MaskBlt which sometimes fails on menus 
		//Explanation: I think I know what the problem is, draw_mask doesnt fail on the first submenus because the menu window is already being shown/exists, but for submenus of those guys the window is not yet shown when we draw onto it, this problaly confuses MaskBlt in some strange way
		urender::draw_mask(maskDC, 0, 0, wDest, hDest, mask, 0, 0, wDest, hDest, colorbr);

		//Draw back to dest with the new content
		BitBlt(destDC, xDest, yDest, wDest, hDest, maskDC, 0, 0, SRCCOPY);

		//Clean up
		SelectObject(maskDC, oldFillBitmap);
		DeleteObject(fillBitmap);
		DeleteDC(maskDC);
	}

//NOTE: the caller is in charge of deleting the hbitmap
//NOTE: color values should be normalized between 0 and 1
//returns 32bpp bitmap
	HBITMAP render_ring(u32 w, u32 h, f32 completion, v4 ring_color, v4 ring_bk_color, v4 inner_circle_color, v4 bk_color) {

		//TODO(fran): this is waaay too slow, we need to either vectorize or go to the gpu

		img _buf;
		img* buf = &_buf;
		buf->width = w;
		buf->height = h;
		buf->bits_per_pixel = 32;
		buf->pitch = buf->width * buf->bits_per_pixel / 8;
		i32 buf_sz = buf->height * buf->pitch;
		buf->mem = malloc(buf_sz); defer{ free(buf->mem); };

		v2 origin{ 0,0 };
		v2 x_axis{ (f32)w,0 };
		v2 y_axis{ 0,(f32)h };

		ring_color.rgb *= ring_color.a; //premultiplication for the color up front
		ring_bk_color.rgb *= ring_bk_color.a;
		inner_circle_color.rgb *= inner_circle_color.a;
		bk_color.rgb *= bk_color.a;

		f32 inv_x_axis_length_sq = 1.f / length_sq(x_axis);
		f32 inv_y_axis_length_sq = 1.f / length_sq(y_axis);

		i32 x_min = buf->width;
		i32 x_max = 0;
		i32 y_min = buf->height;
		i32 y_max = 0;

		v2 points[4] = { origin,origin + x_axis,origin + y_axis,origin + x_axis + y_axis };
		for (v2 p : points) {
			i32 floorx = (i32)floorf(p.x);
			i32 ceilx = (i32)ceilf(p.x);
			i32 floory = (i32)floorf(p.y);
			i32 ceily = (i32)ceilf(p.y);

			if (x_min > floorx)x_min = floorx;
			if (y_min > floory)y_min = floory;
			if (x_max < ceilx)x_max = ceilx;
			if (y_max < ceily)y_max = ceily;
		}

		if (x_min < 0)x_min = 0;
		if (y_min < 0)y_min = 0;
		if (x_max > buf->width)x_max = buf->width;
		if (y_max > buf->height)y_max = buf->height;

		u8* row = (u8*)buf->mem + x_min * (buf->bits_per_pixel / 8)/*bytes per pixel*/ + y_min * buf->pitch;

		for (int y = y_min; y < y_max; y++) {
			u32* pixel = (u32*)row;
			for (int x = x_min; x < x_max; x++) {
				//AARRGGBB

				v2 p = v2{ (f32)x,(f32)y };
				v2 d = p - origin;
				f32 u = dot(d, x_axis) * inv_x_axis_length_sq;
				f32 v = dot(d, y_axis) * inv_y_axis_length_sq;
				if (u >= 0.f && u <= 1.f && v >= 0.f && v <= 1.f) {

					v4 texel_pass1 = main_outter_circle({ u,v }, ring_bk_color.rgb);
					//NOTE: clamp first and then premultiply, this functions sometimes go off the roof on alpha
					texel_pass1 = clamp01(texel_pass1);
					texel_pass1.rgb *= texel_pass1.a; 

					v4 texel_pass2 = main_ring({ u,v }, completion,ring_color.rgb);
					texel_pass2 = clamp01(texel_pass2);
					texel_pass2.rgb *= texel_pass2.a;
					//REMEMBER: clamping is important, before this the ring was basically all black, the main_ring code overflows the 0 to 1 range and that was causing us to loose all of the high bit values

					v4 texel_pass3 = main_inner_circle({ u,v }, inner_circle_color.rgb);
					texel_pass3 = clamp01(texel_pass3);
					texel_pass3.rgb *= texel_pass3.a;

					v4 dest = bk_color;
					
					v4 blended = (1.f - texel_pass1.a) * dest + texel_pass1;

					blended = (1.f - texel_pass2.a) * blended + texel_pass2;

					blended = (1.f - texel_pass3.a) * blended + texel_pass3;

					v4 blended255 = linear1_to_srgb255(blended);

					//if (x == 29 && y == 6) *pixel = 255 << 24 | 255 << 16 | 0; else
					
					*pixel = round_f32_to_i32(blended255.a) << 24 | round_f32_to_i32(blended255.r) << 16 | round_f32_to_i32(blended255.g) << 8 | round_f32_to_i32(blended255.b) << 0; //TODO(fran): should use round_f32_to_u32?

				}
				pixel++;

			}
			row += buf->pitch;
		}


		//Create bitmap
		HBITMAP full_ring = CreateBitmap(buf->width, buf->height, 1, buf->bits_per_pixel, buf->mem);
		return full_ring;
	}

	//NOTE: colors should be in linear1 space
	void draw_ring(HDC destDC, i32 xDest, i32 yDest, i32 wDest, i32 hDest, f32 completion, v4 ring_color, v4 ring_bk_color, v4 inner_circle_color, v4 bk_color, cstr* fontfamily) {
		HBITMAP ring = render_ring(wDest, hDest, completion, ring_color, ring_bk_color, inner_circle_color, bk_color); defer{ DeleteObject(ring); };

		if(fontfamily) { //Text rendering
			HDC tempdc = CreateCompatibleDC(destDC); defer{ DeleteDC(tempdc); };
			HBITMAP oldbmp = (decltype(ring))SelectObject(tempdc, ring); defer{ SelectObject(tempdc, oldbmp); };
			Gdiplus::Graphics graphics(tempdc);//Yeah, gotta change this
			graphics.SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);
			str percentage = to_str((u32)(completion * 100.f));
#if 0
			percentage += L"%";
#endif

			Gdiplus::FontFamily   fontFamily(fontfamily);
			Gdiplus::Font         font(&fontFamily, min(wDest,hDest) * .25f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
			Gdiplus::PointF		  pointF((f32)wDest / 2.f, (f32)hDest / 2.f);
			Gdiplus::StringFormat stringFormat;
			stringFormat.SetAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
			stringFormat.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
			COLORREF txt_color = v4_linear1_to_COLORREF(ring_color);
			Gdiplus::SolidBrush   solidBrush(Gdiplus::Color(GetAValue(txt_color), GetRValue(txt_color), GetGValue(txt_color), GetBValue(txt_color)));

			graphics.DrawString(percentage.c_str(), (INT)percentage.length(), &font, pointF, &stringFormat, &solidBrush);
		}

		draw_bitmap(destDC, xDest, yDest, wDest, hDest, ring, 0, 0, wDest, hDest);
		//TODO(fran): try with different bk colors and with a draw_bitmap function that uses AlphaBlend()
	}

	f32 appropiate_font_h(utf16_str txt,SIZE rc_sz, Gdiplus::Graphics* graphics, Gdiplus::FontFamily* fontFamily, int FontStyle_flags,Gdiplus::Unit unit) {
		f32 res;
		f32 fontsize = (f32)rc_sz.cy;
		Gdiplus::PointF p{0,0};
		Gdiplus::RectF resulting_rc;
		for (int i = 0; i < 10; i++) {
			//TODO(fran): faster algorithm
			Gdiplus::Font font(fontFamily, fontsize, FontStyle_flags, unit);
			graphics->MeasureString(txt.str, (INT)(txt.sz_char() - 1), &font, p, &resulting_rc);
			if ((i32)resulting_rc.Width <= rc_sz.cx && (i32)resulting_rc.Height <= rc_sz.cy) break;
			else {
				f32 dw = safe_ratio0((f32)rc_sz.cx, (f32)resulting_rc.Width);
				f32 dh = safe_ratio0((f32)rc_sz.cy, (f32)resulting_rc.Height);
				f32 d = min(dw, dh);
				fontsize *= d;
				//fontsize *= .9f;
				//decrease 10 percent, not the best solution, we should probably check the diference between resulting_rc and rc_sz, another idea would be to half by two or increase by 50% depending if the resulting_rc is bigger or smaller than desired
			}
		}

		res = fontsize;
		return res;
	}

	enum class txt_align{center,left,right};
	//Renders the text as big as it possibly can in the specified rectangle
	void draw_text_max_coverage(HDC dc, const RECT& r, utf16_str txt, HFONT f, HBRUSH br, txt_align alignment) {
		//TODO(fran): we can do this with gdi
#ifdef UNCAP_GDIPLUS
		int w = RECTW(r), h = RECTH(r);
		Gdiplus::Graphics graphics(dc);
		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);

		LOGFONTW lf;
		int getobjres = GetObject(f, sizeof(lf), &lf); Assert(getobjres == sizeof(lf));

		Gdiplus::FontFamily   fontFamily(lf.lfFaceName);

		int FontStyle_flags = Gdiplus::FontStyle::FontStyleRegular;
		//TODO(fran): we need our own font renderer, from starters the font weight has a much finer control with logfont
		if (lf.lfWeight >= FW_BOLD)FontStyle_flags |= Gdiplus::FontStyle::FontStyleBold;
		if (lf.lfItalic)FontStyle_flags |= Gdiplus::FontStyle::FontStyleItalic;
		if (lf.lfUnderline)FontStyle_flags |= Gdiplus::FontStyle::FontStyleUnderline;
		if (lf.lfStrikeOut)FontStyle_flags |= Gdiplus::FontStyle::FontStyleStrikeout;

		f32 fontsize = appropiate_font_h(txt, { w,h }, &graphics, &fontFamily, FontStyle_flags, Gdiplus::UnitPixel);

		Gdiplus::Font         font(&fontFamily, fontsize, FontStyle_flags, Gdiplus::UnitPixel);

		Gdiplus::StringFormat stringFormat;
		stringFormat.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);//align vertically always

		Gdiplus::PointF		  pointF;
		Gdiplus::StringAlignment str_alignment;
		switch (alignment) {
		case decltype(alignment)::center:
		{
			pointF = { (f32)r.left + (f32)w / 2.f, (f32)r.top + (f32)h / 2.f };
			str_alignment = Gdiplus::StringAlignment::StringAlignmentCenter;
		} break;
		case decltype(alignment)::left:
		{
			pointF = { (f32)r.left, (f32)r.top + (f32)h / 2.f };
			str_alignment = Gdiplus::StringAlignment::StringAlignmentNear;//TODO(fran): im not sure if this is it
		} break;
		case decltype(alignment)::right:
		{
			pointF = { (f32)r.right, (f32)r.top + (f32)h / 2.f };
			str_alignment = Gdiplus::StringAlignment::StringAlignmentFar;//TODO(fran): im not sure if this is it
		} break;
		}

		stringFormat.SetAlignment(str_alignment);

		COLORREF txt_color = ColorFromBrush(br);
		Gdiplus::SolidBrush   solidBrush(Gdiplus::Color(/*GetAValue(txt_color)*/255, GetRValue(txt_color), GetGValue(txt_color), GetBValue(txt_color)));
		graphics.DrawString(txt.str, (INT)(txt.sz_char() - 1), &font, pointF, &stringFormat, &solidBrush);
#endif
	}

	//Intended for a single line of text
	void draw_text(HDC dc, const RECT& r, utf16_str txt, HFONT f,HBRUSH txt_br, HBRUSH bk_br, txt_align alignment, i32 pad_x) {

		HFONT oldfont = SelectFont(dc, f); defer{ SelectFont(dc, oldfont); };
		UINT oldalign = GetTextAlign(dc); defer{ SetTextAlign(dc,oldalign); };

		COLORREF oldtxtcol = SetTextColor(dc, ColorFromBrush(txt_br)); defer{ SetTextColor(dc, oldtxtcol); };
		COLORREF oldbkcol = SetBkColor(dc, ColorFromBrush(bk_br)); defer{ SetBkColor(dc, oldbkcol); };

		TEXTMETRIC tm;
		GetTextMetrics(dc, &tm);
		// Calculate vertical position for the string so that it will be vertically centered
		// We are single line so we want vertical alignment always
		i32 yPos = (r.bottom + r.top - tm.tmHeight) / 2;
		i32 xPos;
		switch (alignment) {
		case decltype(alignment)::center:
		{
			SetTextAlign(dc, TA_CENTER);
			xPos = (r.right - r.left) / 2;
		} break;
		case decltype(alignment)::left:
		{
			SetTextAlign(dc, TA_LEFT);
			xPos = r.left + pad_x;
		} break;
		case decltype(alignment)::right:
		{
			SetTextAlign(dc, TA_RIGHT);
			xPos = r.right -pad_x;
		} break;
		}
		TextOut(dc, xPos, yPos, txt.str, (i32)(txt.sz_char()-1));

		//TODO(fran): TabbedTextOut for completeness
		//TODO(fran): transparent bk color (if possible without gdi+)
	}

	//GDI's RoundRect has no antialiasing
	//TOOD(fran): Fillrect has RECT first and then HBRUSH
	void RoundRectangleFill(HDC dc, HBRUSH br, const RECT& r, u16 radius /*degrees*/)
	{
		//thanks http://codewee.com/view.php?idx=60
		//thanks https://stackoverflow.com/questions/33878184/c-sharp-how-to-make-smooth-arc-region-using-graphics-path
		Gdiplus::Graphics graphics(dc);

		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
		graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
		graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::Color c; c.SetFromCOLORREF(ColorFromBrush(br));
		auto b = Gdiplus::SolidBrush(c);
		Gdiplus::Rect rect(r.left,r.top,RECTW(r),RECTH(r));

		Gdiplus::GraphicsPath path;

		path.AddLine(rect.X + radius, rect.Y, rect.X + rect.Width - (radius * 2), rect.Y);
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y, radius * 2, radius * 2, 270, 90);
		path.AddLine(rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - (radius * 2));
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 0, 90);
		path.AddLine(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
		path.AddArc(rect.X, rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 90, 90);
		path.AddLine(rect.X, rect.Y + rect.Height - (radius * 2), rect.X, rect.Y + radius);
		path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
		path.CloseFigure();

		graphics.FillPath(&b, &path);
	}

	void RoundRectangleCornerFill(HDC dc, HBRUSH br, const RECT& r, u16 radius /*degrees*/)
	{
		//thanks http://codewee.com/view.php?idx=60
		//thanks https://stackoverflow.com/questions/33878184/c-sharp-how-to-make-smooth-arc-region-using-graphics-path
		Gdiplus::Graphics graphics(dc);

		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
		graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
		graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::Color c; c.SetFromCOLORREF(ColorFromBrush(br));
		auto b = Gdiplus::SolidBrush(c);
		Gdiplus::Rect rect(r.left, r.top, RECTW(r), RECTH(r));

		Gdiplus::GraphicsPath path;

		//path.AddLine(rect.X + radius, rect.Y, rect.X + rect.Width - (radius * 2), rect.Y);
		path.StartFigure();
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y, radius * 2, radius * 2, 270, 90);
		//path.AddLine(rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - (radius * 2));
		path.StartFigure();
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 0, 90);
		//path.AddLine(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
		path.StartFigure();
		path.AddArc(rect.X, rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 90, 90);
		//path.AddLine(rect.X, rect.Y + rect.Height - (radius * 2), rect.X, rect.Y + radius);
		path.StartFigure();
		path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
		path.CloseAllFigures();
		path.CloseFigure();

		graphics.FillPath(&b, &path);
	}

	void RoundRectangleArcs(HDC dc, HBRUSH br, const RECT& r, u16 radius /*degrees*/, f32 thickness)
	{
		//thanks http://codewee.com/view.php?idx=60
		//thanks https://stackoverflow.com/questions/33878184/c-sharp-how-to-make-smooth-arc-region-using-graphics-path
		Gdiplus::Graphics g(dc);

		g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
		g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::Color c; c.SetFromCOLORREF(ColorFromBrush(br));
		auto p = Gdiplus::Pen(c, thickness);
		Gdiplus::Rect rect(r.left, r.top, RECTW(r), RECTH(r));
		
		g.DrawArc(&p,rect.X + rect.Width - (radius * 2), rect.Y, radius * 2, radius * 2, 270, 90);
		g.DrawArc(&p,rect.X + rect.Width - (radius * 2), rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 0, 90);
		g.DrawArc(&p,rect.X, rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 90, 90);
		g.DrawArc(&p,rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	}

	void RoundRectangleLines(HDC dc, HBRUSH br, const RECT& r, u16 radius /*degrees*/, f32 thickness)
	{
		//thanks http://codewee.com/view.php?idx=60
		//thanks https://stackoverflow.com/questions/33878184/c-sharp-how-to-make-smooth-arc-region-using-graphics-path
		Gdiplus::Graphics g(dc);

		g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
		g.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::Color c; c.SetFromCOLORREF(ColorFromBrush(br));
		auto p = Gdiplus::Pen(c, thickness);
		Gdiplus::Rect rect(r.left, r.top, RECTW(r), RECTH(r));
		
		g.DrawLine(&p,rect.X + radius, rect.Y, rect.X + rect.Width - radius, rect.Y);
		g.DrawLine(&p,rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - radius);
		g.DrawLine(&p,rect.X + rect.Width - radius, rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
		g.DrawLine(&p,rect.X, rect.Y + rect.Height - radius, rect.X, rect.Y + radius);
	}


	//doesnt look too good really, gdiplus dissapoints again
	//TODO(fran): improve this guy, we need it for big controls, small ones can use RoundRect, I can try rendering the arcs with a smaller pen, or reduce the addarc w and h
	void RoundRectangleBorder(HDC dc, HBRUSH br, const RECT& r, u16 radius /*degrees*/,f32 thickness)
	{
		//semi-thanks http://codewee.com/view.php?idx=60
		//thanks https://stackoverflow.com/questions/33878184/c-sharp-how-to-make-smooth-arc-region-using-graphics-path
		Gdiplus::Graphics graphics(dc);

		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
		graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetMode::PixelOffsetModeHighQuality);
		graphics.SetSmoothingMode(Gdiplus::SmoothingMode::SmoothingModeAntiAlias);

		Gdiplus::Color c; c.SetFromCOLORREF(ColorFromBrush(br));
		auto p = Gdiplus::Pen(c, thickness);
		Gdiplus::Rect rect(r.left, r.top, RECTW(r), RECTH(r));

		Gdiplus::GraphicsPath path;
#if 1
		//TODO(fran): this calculations are completely wrong, the lines dont line up with the arcs, maybe that's part of the problem why it looks so bad
		path.AddLine(rect.X + radius, rect.Y, rect.X + rect.Width - (radius * 2), rect.Y);
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y, radius * 2, radius * 2, 270, 90);
		path.AddLine(rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - (radius * 2));
		path.AddArc(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 0, 90);
		path.AddLine(rect.X + rect.Width - (radius * 2), rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
		path.AddArc(rect.X, rect.Y + rect.Height - (radius * 2), radius * 2, radius * 2, 90, 90);
		path.AddLine(rect.X, rect.Y + rect.Height - (radius * 2), rect.X, rect.Y + radius);
		path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
		path.CloseFigure();
		
		graphics.DrawPath(&p, &path);
#else
		graphics.DrawLine(&p,rect.X + radius, rect.Y, rect.X + rect.Width - (radius * 2), rect.Y);
		graphics.DrawLine(&p,rect.X + rect.Width, rect.Y + radius, rect.X + rect.Width, rect.Y + rect.Height - (radius * 2));
		graphics.DrawLine(&p,rect.X + rect.Width - (radius * 2), rect.Y + rect.Height, rect.X + radius, rect.Y + rect.Height);
		graphics.DrawLine(&p,rect.X, rect.Y + rect.Height - (radius * 2), rect.X, rect.Y + radius);
#endif
	}

	static ULONG_PTR gdiplusToken;//HACK, gdi+ shouldnt need it in the first place but the devs had no idea what they were doing
	void init() {
#ifdef UNCAP_GDIPLUS
		// Initialize GDI+
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif
	}

	void uninit() {
#ifdef UNCAP_GDIPLUS
		//Uninitialize GDI+
		Gdiplus::GdiplusShutdown(gdiplusToken);
#endif
	}

	struct pre_post_main {
		pre_post_main() {
			init();
		}
		~pre_post_main() { //INFO: you can also use the atexit function
			uninit();
		}
	};
	static const pre_post_main PREMAIN_POSTMAIN;
}

/*
void DrawMenuArrow(HDC destDC, RECT& r)
{
	//Thanks again https://www.codeguru.com/cpp/controls/menu/miscellaneous/article.php/c13017/Owner-Drawing-the-Submenu-Arrow.htm
	//NOTE: I dont know if this will be final, dont really wanna depend on windows, but it's pretty good for now. see https://docs.microsoft.com/en-us/windows/win32/gdi/scaling-an-image maybe some of those stretch modes work better than HALFTONE

	//Create the DCs and Bitmaps we will need
	HDC arrowDC = CreateCompatibleDC(destDC);
	HDC fillDC = CreateCompatibleDC(destDC);
	int arrowW = RECTWIDTH(r);
	int arrowH = RECTHEIGHT(r);
	HBITMAP arrowBitmap = CreateCompatibleBitmap(destDC, arrowW, arrowH);
	HBITMAP oldArrowBitmap = (HBITMAP)SelectObject(arrowDC, arrowBitmap);
	HBITMAP fillBitmap = CreateCompatibleBitmap(destDC, arrowW, arrowH);
	HBITMAP oldFillBitmap = (HBITMAP)SelectObject(fillDC, fillBitmap);

	//Set the offscreen arrow rect
	RECT tmpArrowR = rectWH(0, 0, arrowW, arrowH);

	//Draw the frame control arrow (The OS draws this as a black on white bitmap mask)
	DrawFrameControl(arrowDC, &tmpArrowR, DFC_MENU, DFCS_MENUARROW);

	//Set the arrow color
	HBRUSH arrowBrush = unCap_colors.Img;

	//Fill the fill bitmap with the arrow color
	FillRect(fillDC, &tmpArrowR, arrowBrush);

	//Blit the items in a masking fashion
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, fillDC, 0, 0, SRCINVERT);
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, arrowDC, 0, 0, SRCAND);
	BitBlt(destDC, r.left, r.top, arrowW, arrowH, fillDC, 0, 0, SRCINVERT);

	//Clean up
	SelectObject(fillDC, oldFillBitmap);
	DeleteObject(fillBitmap);
	SelectObject(arrowDC, oldArrowBitmap);
	DeleteObject(arrowBitmap);
	DeleteDC(fillDC);
	DeleteDC(arrowDC);
}
*/
/*
void DrawMenuImg(HDC destDC, RECT& r, HBITMAP mask) {
	int imgW = RECTWIDTH(r);
	int imgH = RECTHEIGHT(r);

	HBRUSH imgBr = unCap_colors.Img;
	HBRUSH oldBr = SelectBrush(destDC, imgBr);

	//TODO(fran): I have no idea why I need to pass the "srcDC", no information from it is needed, and the function explicitly says it should be NULL, but if I do it returns false aka error (some error, cause it also doesnt tell you what it is)
	//NOTE: info on creating your own raster ops https://docs.microsoft.com/en-us/windows/win32/gdi/ternary-raster-operations?redirectedfrom=MSDN
	//Thanks https://stackoverflow.com/questions/778666/raster-operator-to-use-for-maskblt
	BOOL res = MaskBlt(destDC, r.left, r.top, imgW, imgH, destDC, 0, 0, mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
	SelectBrush(destDC, oldBr);
}
*/

/*
	void draw_menu_mask(HDC destDC, int xDest, int yDest, int wDest, int hDest, HBITMAP mask, int xSrc, int ySrc, int wSrc, int hSrc, HBRUSH colorbr)
	{
		{BITMAP bmpnfo; GetObject(mask, sizeof(bmpnfo), &bmpnfo); Assert(bmpnfo.bmBitsPixel == 1); }

		//Thanks again https://www.codeguru.com/cpp/controls/menu/miscellaneous/article.php/c13017/Owner-Drawing-the-Submenu-Arrow.htm
		//NOTE: I dont know if this will be final, dont really wanna depend on windows, but it's pretty good for now. see https://docs.microsoft.com/en-us/windows/win32/gdi/scaling-an-image maybe some of those stretch modes work better than HALFTONE

		//Create the DCs and Bitmaps we will need
		HDC maskDC = CreateCompatibleDC(destDC);
		HDC fillDC = CreateCompatibleDC(destDC);
		HBITMAP fillBitmap = CreateCompatibleBitmap(destDC, wDest, hDest);
		HBITMAP oldFillBitmap = (HBITMAP)SelectObject(fillDC, fillBitmap);


		HBITMAP scaled_mask = urender::scale_mask(mask, wDest, hDest);
		HBITMAP oldArrowBitmap = (HBITMAP)SelectObject(maskDC, scaled_mask);
		BitBlt(maskDC, 0, 0, wDest, hDest, maskDC, 0, 0, DSTINVERT);

		//Set the arrow color
		HBRUSH arrowBrush = colorbr;

		//Set the offscreen arrow rect
		RECT tmpArrowR = rectWH(0, 0, wDest, hDest);

		//Fill the fill bitmap with the arrow color
		FillRect(fillDC, &tmpArrowR, arrowBrush);

		//Blit the items in a masking fashion //TODO(fran): this doesnt blend correctly
		BitBlt(destDC, xDest, yDest, wDest, hDest, fillDC, 0, 0, SRCINVERT);
		BitBlt(destDC, xDest, yDest, wDest, hDest, maskDC, 0, 0, SRCAND);
		BitBlt(destDC, xDest, yDest, wDest, hDest, fillDC, 0, 0, SRCINVERT);

		//Clean up
		SelectObject(fillDC, oldFillBitmap);
		DeleteObject(fillBitmap);
		SelectObject(maskDC, oldArrowBitmap);
		DeleteObject(scaled_mask);
		DeleteDC(fillDC);
		DeleteDC(maskDC);
	}
*/

