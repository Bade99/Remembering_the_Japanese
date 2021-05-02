#pragma once
#include "windows_sdk.h"
#include "win32_Platform.h"
#include "win32_Helpers.h"
#include "win32_Renderer.h"
#include "unCap_Math.h"

//------------------"API"------------------:
#define score_base_msg_addr (WM_USER + 4000)
#define SC_SETSCORE (score_base_msg_addr+1) /*wparam = float ; lparam = unused*/ /*score should be normalized between 0.0 and 1.0*/ /*IMPORTANT: use f32_to_WPARAM() from the "Helpers" file, casting (eg. (WPARAM).7f) wont work*/

//TODO(fran): BUG: we are drawing the full score on the first frame since we ask for repaint on SC_SETSCORE, we should probably ask to start the anim there

#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
//#include <dxgi1_3.h>
//#include <d3d11_3.h>
#include <d2d1effectauthor.h>  
#include <d2d1effecthelpers.h>
#include <initguid.h> //so that DEFINE_GUID actually maps to something
#pragma comment(lib,"d2d1.lib")
template<typename Interface>
void SafeRelease(Interface** ppInterfaceToRelease) {
	if (*ppInterfaceToRelease != NULL) {
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

namespace score {

#define ANIM_SCORE_ring 1

	
	struct ProcState {
		HWND wnd;
		HWND nc_parent;//NOTE: we dont probably care about interacting with the parent, this is just visual candy
		f32 score;
		HFONT font;

		//HBITMAP score; //TODO(fran): we probably dont want to be redrawing the score bitmap on each resize
		//bool redraw_score;
		//RECT last_redraw;//Stores RECT size for the last time a redraw happened

		struct {
			HBRUSH bk;
			HBRUSH ring_bk;
			HBRUSH ringfull;
			HBRUSH ringempty;
			HBRUSH inner_circle;
		} brushes;

		struct {
			struct {
				f32 score_start;
				f32 score;//total score, last frame will present this value, in between frames will interpolate from score_start, also it must match ProcState->score or the animation will stop (this will be important when multithreading)
				u32 frames_cnt;//total frames
				u32 frame_idx;//current frame
			} ring;

		}anim;

		ID2D1Factory1* factory;//NOTE: ID2D1Factory doesnt have createdevice() which apparently is needed to be able to use effects
		//IDXGIFactory2* dxgi_factory;
		ID2D1HwndRenderTarget* rendertarget;
		ID2D1Effect* effect_ringlifebar;

		/*ID3D11Device2* d3d_device;
		ID3D11DeviceContext3* d3d_context;
		IDXGISwapChain1* swapchain;*/
	};

	constexpr cstr wndclass[] = L"がいじんの_wndclass_score";

	ProcState* get_state(HWND wnd) {
		ProcState* state = (ProcState*)GetWindowLongPtr(wnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
		return state;
	}

	void set_state(HWND wnd, ProcState* state) {
		SetWindowLongPtr(wnd, 0, (LONG_PTR)state);
	}

	//NOTE: any NULL HBRUSH remains unchanged
	//NOTE: interpolation happens between ringfull and ringempty, using the score percentage to get the color
	void set_brushes(HWND wnd, BOOL repaint, HBRUSH bk, HBRUSH ring_bk, HBRUSH ringfull, HBRUSH ringempty, HBRUSH inner_circle) {
		ProcState* state = get_state(wnd);
		if (state) {
			if (bk) state->brushes.bk = bk;
			if (ring_bk) state->brushes.ring_bk = ring_bk;
			if (ringfull) state->brushes.ringfull = ringfull;
			if (ringempty) state->brushes.ringempty = ringempty;
			if (inner_circle) state->brushes.inner_circle = inner_circle;
			if (repaint)InvalidateRect(state->wnd, NULL, TRUE);
		}
	}

	//void resize(ProcState* state){
		//left as a TODO if we want to reduce bitmap redraws
	//	InvalidateRect(state->wnd, NULL, TRUE);
	//}

	void play_anim(HWND hwnd, UINT /*msg*/, UINT_PTR anim_id, DWORD /*sys_elapsed*/) {
		ProcState* state = get_state(hwnd);
		bool next_frame = false;

		switch (anim_id) {
		case ANIM_SCORE_ring:
		{
			auto& anim = state->anim.ring;
			anim.frame_idx++;
			if (anim.frame_idx < anim.frames_cnt) next_frame = true;

			f32 dt = (f32)anim.frame_idx / (f32)anim.frames_cnt;

			f32 anim_score = lerp(anim.score_start, dt, anim.score);

			//HACK: (we cant match score this way)
			PostMessage(state->wnd, SC_SETSCORE, f32_to_WPARAM(anim_score), 0);

		} break;
		}

		if (next_frame) {
			SetTimer(state->wnd, anim_id, max((u32)1, (u32)((1.f/(f32)win32_get_refresh_rate_hz(state->wnd))*1000)), play_anim);
		}
		else {
			KillTimer(state->wnd, anim_id);
		}
	}

	void start_anim(ProcState* state, u64 anim_id) {
		switch (anim_id) {
		case ANIM_SCORE_ring:
		{
			state->anim.ring.score_start = 0.f;
			state->anim.ring.score = state->score;
			u32 refresh = win32_get_refresh_rate_hz(state->wnd);
			state->anim.ring.frames_cnt = refresh;
			state->anim.ring.frame_idx = 0;
		} break;
		}
		SetTimer(state->wnd, anim_id, USER_TIMER_MINIMUM, play_anim);
	}


	DEFINE_GUID(GUID_PixelShader_RingLifeBar, 0x10000000, 0x1000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#include "Pix_RingLifeBar.h"

#if 0 //we'll build the transform right inside the effect
	class Transform_RingLifeBar : public ID2D1DrawTransform {
	public:
		Transform_RingLifeBar();

		IFACEMETHODIMP_(UINT32) GetInputCount() const { return 1; }

		IFACEMETHODIMP MapInputRectsToOutputRect( const D2D1_RECT_L* input_rcs, const D2D1_RECT_L* input_opaque_subrcs, UINT32 input_rc_cnt, D2D1_RECT_L* output_rc, D2D1_RECT_L* output_opaque_subrc) {
			// This transform is designed to only accept one input.
			if (input_rc_cnt != GetInputCount()) return E_INVALIDARG;

			// The output of the transform will be the same size as the input.
			*output_rc = input_rcs[0];
			// Indicate that the image's opacity has not changed.
			*output_opaque_subrc = input_opaque_subrcs[0];
			// The size of the input image can be saved here for subsequent operations.
			//m_inputRect = input_rcs[0];

			return S_OK;
		}

		IFACEMETHODIMP MapOutputRectToInputRects( const D2D1_RECT_L* output_rc, D2D1_RECT_L* input_rcs, UINT32 input_rc_cnt) const {
			// This transform is designed to only accept one input.
			if (input_rc_cnt != GetInputCount()) return E_INVALIDARG;

			// The input needed for the transform is the same as the visible output.
			input_rcs[0] = *output_rc;
			return S_OK;
		}

		IFACEMETHODIMP MapInvalidRect( UINT32 input_idx, D2D1_RECT_L invalid_input_rc, D2D1_RECT_L* invalid_output_rc) const {
			// This transform is designed to only accept one input.
			if (input_idx != 0) { return E_INVALIDARG; }

			// If part of the transform's input is invalid, mark the corresponding output region as invalid. 
			*invalid_output_rc = invalid_input_rc;
			return S_OK;
		}

		//Draw transform specific function
		//Called by d2d when the transform is first added to an effect's transform graph
		HRESULT SetDrawInfo(ID2D1DrawInfo* _draw_nfo) { draw_nfo = _draw_nfo; } //NOTE: we can modify this guy on property setters now that we store it as a member variable

		// IUnknown Methods:
		IFACEMETHODIMP_(ULONG) AddRef();
		IFACEMETHODIMP_(ULONG) Release();
		IFACEMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppOutput);

	private:
		LONG ref_cnt; // Internal ref count used by AddRef() and Release() methods.
		//D2D1_RECT_L m_inputRect; // Stores the size of the input image.
		ID2D1DrawInfo* draw_nfo;
	};
#endif

	//TODO(fran): replace TEXT with L
#define XML(X) TEXT(#X)
	DEFINE_GUID(CLSID_Effect_RingLifeBar, 0x10000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	class Effect_RingLifeBar : public ID2D1EffectImpl, public ID2D1DrawTransform {
	public:
		//Called by direct2d after the application calls createeffect()
		//Here you can perform internal intialization and create the transform graph
		IFACEMETHODIMP Initialize(ID2D1EffectContext* context, ID2D1TransformGraph* transform_graph) {

			HRESULT hr = context->LoadPixelShader(GUID_PixelShader_RingLifeBar, Pix_RingLifeBar, sizeof(Pix_RingLifeBar));

			// This loads the shader into the Direct2D image effects system and associates it with the GUID passed in.
			// If this method is called more than once (say by other instances of the effect) with the same GUID, the system will simply do nothing, ensuring that only one instance of a shader is stored regardless of how many times it is used.
			if (SUCCEEDED(hr))
			{
				// The graph consists of a single transform. In fact, this class is the transform, reducing the complexity of implementing an effect when all we need to do is use a single pixel shader.
				// Connects the effect's input to the transform's input, and connects the transform's output to the effect's output.
				hr = transform_graph->SetSingleTransformNode(this);
			}

			return hr;

			return hr;
		}

		//Performs operations in response to external changes
		//Called by d2d just before rendering if: 
		//	-the effect has been initialized but not drawn
		//	-an effect property has changed since the last render
		//	-the state of the d2d context has changed since the last render (eg dpi)
		IFACEMETHODIMP PrepareForRender(D2D1_CHANGE_TYPE change_type) {
			//TODO(fran): what about the change_type?
			return draw_nfo->SetPixelShaderConstantBuffer((BYTE*)&constants, sizeof(constants));
		}
		//Called when the number of inputs is changed (cause some effects support variable nº of inputs)
		//If the effect does not support a variable input count, this method can simply return E_NOTIMPL
		IFACEMETHODIMP SetGraph(ID2D1TransformGraph* graph) { return E_NOTIMPL; };

		enum prop_idx : u32 {
			ring_color = 0,
			ring_bk_color,
			inner_circle_color,
			bk_color,
			score,
		};

		//Effect must be registered before first instantiated, the registration is scoped to a d2d factory instance
		//This is where you need the unique GUID, this GUID is used when calling CreateEffect
		static HRESULT Register(ID2D1Factory1* factory) {
			const utf16* properties =
				XML(
					<?xml version='1.0'?>
					<Effect>
						<!--System Properties-->
						<Property name='DisplayName' type='string' value='RingLifeBar'/>
						<Property name='Author' type='string' value='fran'/>
						<Property name='Category' type='string' value='Bar'/>
						<Property name='Description' type='string' value='Ring lifebar subnautica style.'/>
						<Inputs>
							<Input name='Source'/>
						</Inputs>
						<!--Custom Properties-->
			            <Property name='ring_color' type='vector4'>
							<Property name='DisplayName' type='string' value='Ring Color'/>
							<Property name='Default' type='vector4' value='(0.0, 0.0, 0.0, 0.0)' />
						</Property>
						<Property name='ring_bk_color' type='vector4'>
							<Property name='DisplayName' type='string' value='Ring Bk Color'/>
							<Property name='Default' type='vector4' value='(0.0, 0.0, 0.0, 0.0)' />
						</Property>
						<Property name='inner_circle_color' type='vector4'>
							<Property name='DisplayName' type='string' value='Inner Circle Color'/>
							<Property name='Default' type='vector4' value='(0.0, 0.0, 0.0, 0.0)' />
						</Property>
						<Property name='bk_color' type='vector4'>
							<Property name='DisplayName' type='string' value='Bk Color'/>
							<Property name='Default' type='vector4' value='(0.0, 0.0, 0.0, 0.0)' />
						</Property>
						<Property name='score' type='float'>
							<Property name='DisplayName' type='string' value='Score'/>
							<Property name='Default' type='float' value='0.0' />
						</Property>

					</Effect>
				);

			const D2D1_PROPERTY_BINDING bindings[] =
			{
				D2D1_VALUE_TYPE_BINDING(L"ring_color", &set_ring_color, &get_ring_color),
				D2D1_VALUE_TYPE_BINDING(L"ring_bk_color", &set_ring_bk_color, &get_ring_bk_color),
				D2D1_VALUE_TYPE_BINDING(L"inner_circle_color", &set_inner_circle_color, &get_inner_circle_color),
				D2D1_VALUE_TYPE_BINDING(L"bk_color", &set_bk_color, &get_bk_color),
				D2D1_VALUE_TYPE_BINDING(
					L"score",      // The name of property. Must match name attribute in XML.
					&set_score,     // The setter method that is called on "SetValue".
					&get_score      // The getter method that is called on "GetValue".
				),
			};
			
			auto res = factory->RegisterEffectFromString(CLSID_Effect_RingLifeBar, properties, bindings, ARRAYSIZE(bindings), CreateEffect);
			Assert(SUCCEEDED(res));
			return res;
		}
		static HRESULT __stdcall CreateEffect(IUnknown** effect_impl) {
			// This code assumes that the effect class initializes its reference count to 1. //TODO(fran): I think it wants me to refCount++ on the constructor
			*effect_impl = static_cast<ID2D1EffectImpl*>(new Effect_RingLifeBar());

			if (*effect_impl == nullptr) return E_OUTOFMEMORY;

			return S_OK;
		}

		//----------Transform----------::
		IFACEMETHODIMP_(UINT32) GetInputCount() const { return 1; }

		IFACEMETHODIMP MapInputRectsToOutputRect(const D2D1_RECT_L* input_rcs, const D2D1_RECT_L* input_opaque_subrcs, UINT32 input_rc_cnt, D2D1_RECT_L* output_rc, D2D1_RECT_L* output_opaque_subrc) {
			// This transform is designed to only accept one input.
			if (input_rc_cnt != GetInputCount()) return E_INVALIDARG;

			// The output of the transform will be the same size as the input.
			*output_rc = input_rcs[0];
			// Indicate that the image's opacity has not changed.
			*output_opaque_subrc = input_opaque_subrcs[0];
			// The size of the input image can be saved here for subsequent operations.
			//m_inputRect = input_rcs[0];

			return S_OK;
		}

		IFACEMETHODIMP MapOutputRectToInputRects(const D2D1_RECT_L* output_rc, D2D1_RECT_L* input_rcs, UINT32 input_rc_cnt) const {
			// This transform is designed to only accept one input.
			if (input_rc_cnt != GetInputCount()) return E_INVALIDARG;

			// The input needed for the transform is the same as the visible output.
			input_rcs[0] = *output_rc;
			return S_OK;
		}

		IFACEMETHODIMP MapInvalidRect(UINT32 input_idx, D2D1_RECT_L invalid_input_rc, D2D1_RECT_L* invalid_output_rc) const {
			// This transform is designed to only accept one input.
			if (input_idx != 0) { return E_INVALIDARG; }

			// If part of the transform's input is invalid, mark the corresponding output region as invalid. 
			*invalid_output_rc = invalid_input_rc;
			return S_OK;
		}

		//Draw transform specific function
		//Called by d2d when the transform is first added to an effect's transform graph
		HRESULT SetDrawInfo(ID2D1DrawInfo* _draw_nfo) {
			draw_nfo = _draw_nfo;  //NOTE: we can modify this guy on property setters now that we store it as a member variable
			return draw_nfo->SetPixelShader(GUID_PixelShader_RingLifeBar);
		}


		IFACEMETHODIMP_(ULONG) AddRef() { return ++ref_cnt; }
		IFACEMETHODIMP_(ULONG) Release() {
			if (--ref_cnt == 0) {
				delete this;
				return 0;
			}
			else {
				return ref_cnt;
			}
		}
		// This enables the stack of parent interfaces to be queried. In this instance this method simply enables the user to cast to an ID2D1EffectImpl or IUnknown instance.
		IFACEMETHODIMP QueryInterface( REFIID riid, void** output) {
			*output = nullptr;
			HRESULT hr = S_OK;

			if (riid == __uuidof(ID2D1EffectImpl)) *output = reinterpret_cast<ID2D1EffectImpl*>(this);
			else if (riid == __uuidof(ID2D1DrawTransform)) *output = static_cast<ID2D1DrawTransform*>(this);
			else if (riid == __uuidof(ID2D1Transform)) *output = static_cast<ID2D1Transform*>(this);
			else if (riid == __uuidof(ID2D1TransformNode)) *output = static_cast<ID2D1TransformNode*>(this);
			else if (riid == __uuidof(IUnknown)) *output = this;
			else hr = E_NOINTERFACE;

			if (*output != nullptr) AddRef();

			return hr;
		}


		HRESULT set_score(f32 _score) { constants.score = clamp01(_score); return S_OK; }
		f32 get_score() const { return constants.score; }

		HRESULT set_ring_color(D2D_VECTOR_4F _ring_color) { constants.ring_color = _ring_color; return S_OK; }
		D2D_VECTOR_4F get_ring_color() const { return constants.ring_color; }

		HRESULT set_ring_bk_color(D2D_VECTOR_4F _ring_bk_color) { constants.ring_bk_color = _ring_bk_color; return S_OK; }
		D2D_VECTOR_4F get_ring_bk_color() const { return constants.ring_bk_color; }

		HRESULT set_inner_circle_color(D2D_VECTOR_4F _inner_circle_color) { constants.inner_circle_color = _inner_circle_color; return S_OK; }
		D2D_VECTOR_4F get_inner_circle_color() const { return constants.inner_circle_color; }

		HRESULT set_bk_color(D2D_VECTOR_4F _bk_color) { constants.bk_color = _bk_color; return S_OK; }
		D2D_VECTOR_4F get_bk_color() const { return constants.bk_color; }

	private:
		Effect_RingLifeBar() : ref_cnt(0), draw_nfo(0), constants{0} {}

		LONG ref_cnt;
		//NOTE: if you need to you can store the transform_graph to be used in other functions such as prepareforrender()

		//D2D1_RECT_L m_inputRect; // Stores the size of the input image.
		ID2D1DrawInfo* draw_nfo; //Transform data

		struct {
			D2D_VECTOR_4F ring_color, ring_bk_color, inner_circle_color, bk_color;
			f32 score;
		} constants;
	};


	HRESULT CreateDeviceResources(ProcState* state) {
		HRESULT hr = S_OK;

		if (!state->rendertarget) {
			RECT rc; GetClientRect(state->wnd, &rc);

			D2D1_SIZE_U size{ (u32)RECTW(rc), (u32)RECTH(rc) };

			D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_PREMULTIPLIED
			);

			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
#if 0
			props.pixelFormat = pixelFormat;//this exact thing gets set by default (bgra and premultiplied)
#endif
			hr = state->factory->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(state->wnd, size), &state->rendertarget);
			Assert(SUCCEEDED(hr));

			//TODO(fran): this should probably go in a separate if statement
			ID2D1DeviceContext* device_ctx; state->rendertarget->QueryInterface(&device_ctx); Assert(device_ctx);
			hr = device_ctx->CreateEffect(CLSID_Effect_RingLifeBar, &state->effect_ringlifebar); Assert(SUCCEEDED(hr));
			Assert(state->effect_ringlifebar);
			device_ctx->Release();
		}
		return hr;
	}

	void DiscardDeviceResources(ProcState* state) {
		SafeRelease(&state->rendertarget);
		SafeRelease(&state->effect_ringlifebar);
	}

	void on_resize(ProcState* state) {
		if (state->rendertarget) {
			RECT rc; GetClientRect(state->wnd, &rc);
			//NOTE: This method can fail, but it's okay to ignore the error here, because the error will be returned again the next time EndDraw is called.
			state->rendertarget->Resize(D2D1_SIZE_U{ (u32)RECTW(rc), (u32)RECTH(rc) });
		}
	}

	void on_render(ProcState* state) {
		HRESULT hr/* = S_OK*/;

		hr = CreateDeviceResources(state);

		if (SUCCEEDED(hr)) {
			state->rendertarget->BeginDraw();

			state->rendertarget->SetTransform(D2D1::Matrix3x2F::Identity());

			D2D1_COLOR_F clearcol;//rgba
			if constexpr(1)
			{
				COLORREF col = ColorFromBrush(state->brushes.bk);
				f32 inv255 = 1.f / 255.f;
				clearcol.r = GetGValue(col) * inv255;
				clearcol.g = GetRValue(col) * inv255;
				clearcol.b = GetBValue(col) * inv255;
				clearcol.a = 1.f; 
			}
			else clearcol = D2D1::ColorF(D2D1::ColorF::White);
			state->rendertarget->Clear(clearcol);

			D2D1_SIZE_F rtSize = state->rendertarget->GetSize();

			//Draw

			// Obtain hwndRenderTarget's deviceContext
			ID2D1DeviceContext* device_ctx; state->rendertarget->QueryInterface(&device_ctx); Assert(device_ctx);
			
			ID2D1BitmapRenderTarget* bitmapRenderTarget; 
			state->rendertarget->CreateCompatibleRenderTarget(&bitmapRenderTarget);

			ID2D1Bitmap* bitmap;
			bitmapRenderTarget->GetBitmap(&bitmap);

			//NOTE: D2D_VECTOR_4F xyzw == rgba
			auto v4_to_D2D_VECTOR_4F = [](v4 v) {return D2D_VECTOR_4F{ v.x,v.y,v.z,v.w }; };

			v4 ringfull = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ringfull), 255);
			v4 ringempty = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ringempty), 255);
			D2D_VECTOR_4F ring_color = v4_to_D2D_VECTOR_4F(lerp(ringempty, state->score, ringfull));//NOTE: lerping should only be done on already premultiplied values, since we're using 255 we're ok for now
			D2D_VECTOR_4F ring_bk = v4_to_D2D_VECTOR_4F(COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ring_bk), 255));
			D2D_VECTOR_4F inner_circle = v4_to_D2D_VECTOR_4F(COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.inner_circle), 255));
			D2D_VECTOR_4F bk = v4_to_D2D_VECTOR_4F(COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.bk), 255));
			

			state->effect_ringlifebar->SetInput(0, bitmap);
			state->effect_ringlifebar->SetValue(Effect_RingLifeBar::prop_idx::score, state->score);
			state->effect_ringlifebar->SetValue(Effect_RingLifeBar::prop_idx::ring_color, ring_color);
			state->effect_ringlifebar->SetValue(Effect_RingLifeBar::prop_idx::ring_bk_color, ring_bk);
			state->effect_ringlifebar->SetValue(Effect_RingLifeBar::prop_idx::inner_circle_color, inner_circle);
			state->effect_ringlifebar->SetValue(Effect_RingLifeBar::prop_idx::bk_color, bk);

			// Draw resulting bitmap (I think it draws onto state->rendertarget)
			device_ctx->DrawImage(state->effect_ringlifebar, D2D1_INTERPOLATION_MODE_LINEAR);

			//TODO(fran): render font

			//ID2D1Image* outputimg; defer{ outputimg->Release(); };
			//state->effect_ringlifebar->GetOutput(&outputimg); Assert(outputimg);

			//state->rendertarget->DrawBitmap();

			bitmap->Release();
			bitmapRenderTarget->Release();
			device_ctx->Release();
			hr = state->rendertarget->EndDraw();
			if (hr == D2DERR_RECREATE_TARGET) {
				hr = S_OK;
				DiscardDeviceResources(state);
			}
		}
	}

	void ask_for_repaint(ProcState* state) { InvalidateRect(state->wnd, NULL, FALSE); }//NOTE: directx docs told me to put FALSE instead of TRUE, maybe that avoids sending wm_erasebackground

#if 0
#include <wrl.h>
	//directx device independent resources (can last for the duration of the application)
	void CreateDeviceIndependentResources(ProcState* state) {
		auto res = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state->factory); Assert(res == S_OK);//TODO(fran): runtime assert

		res = CreateDXGIFactory2(0, __uuidof(decltype(*state->dxgifactory)), (void**)&state->dxgifactory); Assert(res == S_OK);//TODO(fran): runtime assert
		state->dxgifactory->CreateSwapChainForHwnd(, , );
		state->factory->CreateDevice(, );


		// This flag adds support for surfaces with a different color channel ordering than the API default. It is required for compatibility with Direct2D.
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG) && 0
		if (DX::SdkLayersAvailable())
		{
			// If the project is in a debug build, enable debugging via SDK Layers with this flag.
			creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
		}
#endif

		// This array defines the set of DirectX hardware feature levels this app will support.
		// Note the ordering should be preserved.
		// Don't forget to declare your application's minimum required feature level in its description.  All applications are assumed to support 9.1 unless otherwise stated.
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};

		// Create the Direct3D 11 API device object and a corresponding context.
		using namespace Microsoft::WRL;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		D3D_FEATURE_LEVEL d3dFeatureLevel;

		HRESULT hr = D3D11CreateDevice(
			nullptr,                    // Specify nullptr to use the default adapter.
			D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
			0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
			creationFlags,              // Set debug and Direct2D compatibility flags.
			featureLevels,              // List of feature levels this app can support.
			ARRAYSIZE(featureLevels),   // Size of the list above.
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Runtime apps.
			&device,                    // Returns the Direct3D device created.
			&d3dFeatureLevel,         // Returns feature level of device created.
			&context                    // Returns the device immediate context.
		);
		if (FAILED(hr)) {
			// If the initialization fails, fall back to the WARP device.
			// For more information on WARP, see: 
			// http://go.microsoft.com/fwlink/?LinkId=286690
			hr = D3D11CreateDevice(
					nullptr, D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
					0, creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device, &d3dFeatureLevel, &context);
			runtime_assert(SUCCEEDED(hr), L"Failed to create D3D Device");
		}

		// Store pointers to the Direct3D 11.1 API device and immediate context.
		device.As(&state->d3dDevice);

		DX::ThrowIfFailed(
			context.As(&m_d3dContext)
		);

		// Create the Direct2D device object and a corresponding context.
		ComPtr<IDXGIDevice3> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
		);

		DX::ThrowIfFailed(
			m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

		DX::ThrowIfFailed(
			m_d2dDevice->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				&m_d2dContext
			)
		);

	}
#endif

	LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		ProcState* state = get_state(hwnd);
		switch (msg) {
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			decltype(state) state = (decltype(state))calloc(1, sizeof(decltype(*state)));
			Assert(state);
			state->nc_parent = create_nfo->hwndParent;
			state->wnd = hwnd;
			
			auto res = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state->factory); runtime_assert(SUCCEEDED(res), L"Failed to Create D2D Factory");
			
			Effect_RingLifeBar::Register(state->factory);//Register shader/effect on factory

			//CreateDeviceIndependentResources(state);

			set_state(hwnd, state);
			return TRUE;
		} break;
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			return 0;
		} break;
		case WM_SIZE:
		{
			on_resize(state);
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_DISPLAYCHANGE:
		{
			ask_for_repaint(state);
			return 0;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				SafeRelease(&state->factory);
				SafeRelease(&state->rendertarget);
				//SafeRelease(&state->effect_ringlifebar); //TODO(fran): we should be able to release this but it crashes god knows why
				free(state);
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_DESTROY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCCALCSIZE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SHOWWINDOW:
		{
			BOOL show = (BOOL)wparam;
			if (show) {
				//Start animation
				start_anim(state, ANIM_SCORE_ring);
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_GETTEXT:
		{
			return 0;
		} break;
		case WM_GETTEXTLENGTH:
		{
			return 0;
		} break;
		case WM_IME_SETCONTEXT://sent the first time on SetFocus
		{
			return 0; //We dont want IME
		} break;
		case WM_IME_NOTIFY://for some reason you still get this even when not doing WM_IME_SETCONTEXT
		{
			return 0;
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCPAINT:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case SC_SETSCORE:
		{
			f32 new_score = f32_from_WPARAM(wparam);
			state->score = clamp01(new_score);
			//TODO(fran): it may be good to check whether new_score == old_score to avoid re-rendering
			//TODO(fran): if we're already visible we should ask for an animation start from our prev_score
			InvalidateRect(state->wnd, NULL, TRUE);
			return 0;
		} break;
		case WM_ERASEBKGND:
		{
#if 0
			LRESULT res;
			if (state->brushes.bk) {
				HDC dc = (HDC)wparam;
				RECT r; GetClientRect(state->wnd, &r);
#if 0
				FillRect(dc, &r, state->brushes.bk);
#else
				FillRectAlpha(dc, &r, state->brushes.bk, 255); //TODO(fran): this function probably needs to premultiply the alpha, which it currently does not
#endif
				res = 1;//We erased the bk
			}
			else res = DefWindowProc(hwnd, msg, wparam, lparam);
			return res;
#else
			return 0;//WM_PAINT will take care of everything
#endif
		} break;
		case WM_PAINT:
		{
#if 0
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(hwnd, &ps); };

			RECT rc;  GetClientRect(state->wnd, &rc);
			int score_x, score_y, score_w, score_h;
			score_w = score_h = min(RECTW(rc), RECTH(rc));
			score_x = (RECTW(rc) - score_w) / 2;
			score_y = (RECTH(rc) - score_h) / 2;

			v4 ringfull = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ringfull), 255);
			v4 ringempty = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ringempty), 255);
			v4 ring_color = lerp(ringempty, state->score, ringfull);//NOTE: lerping should only be done on aldready premultiplied values, since we're using 255 we're ok for now
			v4 ring_bk = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.ring_bk), 255);
			v4 inner_circle = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.inner_circle), 255);
			v4 bk = COLORREF_to_v4_linear1(ColorFromBrush(state->brushes.bk), 255);

			LOGFONT lf; int getobjres = GetObject(state->font, sizeof(lf), &lf); Assert(getobjres == sizeof(lf));

			urender::draw_ring(dc, score_x, score_y, score_w, score_h, state->score, ring_color, ring_bk, inner_circle, bk,lf.lfFaceName);

			//TODO(fran): fill in the unpainted parts with the bk color
#else
			on_render(state);
			ValidateRect(state->wnd, NULL);//this is usually done by BeginPaint
#endif
			return 0;
		} break;
		case WM_NCHITTEST:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETCURSOR:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEMOVE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_MOUSEACTIVATE:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_LBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONDOWN:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_RBUTTONUP:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_PARENTNOTIFY:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_KILLFOCUS:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_SETFONT:
		{
			HFONT new_font = (HFONT)wparam;
			BOOL redraw = LOWORD(lparam);
			state->font = new_font;
			if(redraw)InvalidateRect(state->wnd, NULL, TRUE);
			return 0;
		} break;
		default:
#ifdef _DEBUG
			Assert(0);
#else 
			return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
		}
		return 0;
	}

	void init_wndclass(HINSTANCE inst) { //INFO: now that we use pre_post_main we cant depend on anything that isnt calculated at compile time
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = Proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = 0; //TODO(fran): unCap_colors.ControlBk hasnt been deserialized by the point pre_post_main gets executed!
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = wndclass;
		wcex.hIconSm = 0;

		ATOM class_atom = RegisterClassExW(&wcex);
		Assert(class_atom);
	}

	struct pre_post_main {
		pre_post_main() {
			init_wndclass(GetModuleHandleW(NULL));
		}
		~pre_post_main() { //INFO: you can also use the atexit function
			//Classes are de-registered automatically by the os
		}
	};
	static const pre_post_main PREMAIN_POSTMAIN;

}