#include "stdafx.h"
#include "CamGrabber.h"
#include <string>
#pragma comment(lib, "strmiids")

HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
void MyFreeMediaType(AM_MEDIA_TYPE& mt);

const CLSID CLSID_nullRenderer = { 0xC1F400A4, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

CamGrabber::CamGrabber()
{
}


CamGrabber::~CamGrabber()
{
}


bool CamGrabber::Init(HWND hWnd)
{
	m_hWnd = hWnd;

	HRESULT hr = m_captureGraphBuilder.CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) return false;

	hr = m_graphBuilder.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) return false;

	// Initialize the Capture Graph Builder.
	hr = m_captureGraphBuilder->SetFiltergraph(m_graphBuilder);
	if (FAILED(hr)) return false;

	DWORD dwRegister;
	hr = AddToRot(m_graphBuilder, &dwRegister);
	if (FAILED(hr)) return false;

	// find capture device
	CComPtr<ICreateDevEnum> devEnum;
	CComPtr<IEnumMoniker> enumMoniker;

	// Create the System Device Enumerator.
	hr = devEnum.CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
	if (FAILED(hr)) return false;

	// Create an enumerator for the video capture category.
	hr = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enumMoniker, 0);
	if (FAILED(hr)) return false;
	if (hr == S_FALSE) return false;

	CComPtr<IMoniker> Moniker;
	while (enumMoniker->Next(1, &Moniker, NULL) == S_OK)
	{
		CComPtr<IPropertyBag> PropBag;
		hr = Moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&PropBag));
		if (FAILED(hr))
		{
			Moniker.Release();
			continue;  // Skip this one, maybe the next one will work.
		}
		// Find the description or friendly name.
		VARIANT varName;
		VariantInit(&varName);
		hr = PropBag->Read(L"Description", &varName, 0);
		if (FAILED(hr)) hr = PropBag->Read(L"FriendlyName", &varName, 0); // retry
		if (FAILED(hr))
		{
			PropBag.Release();
			Moniker.Release();
			continue;  // Skip this one, maybe the next one will work.
		}

		// Add it to the application's list box.
		USES_CONVERSION;
		std::wstring name;
		name = OLE2T(varName.bstrVal);
		VariantClear(&varName);
		if (name == L"OEM Device") // TODO: add select
		{
			hr = Moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_sourceFilter);
			if (FAILED(hr)) return false;

			hr = m_graphBuilder->AddFilter(m_sourceFilter, L"Capture XW Filter");
			if (FAILED(hr)) return false;
		}

		PropBag.Release();
		Moniker.Release();
	}

	// add sample grabber
	CComPtr<IBaseFilter> sampleGrabber;
	hr = sampleGrabber.CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC);
	if (FAILED(hr)) return false;

	hr = sampleGrabber->QueryInterface(IID_ISampleGrabber, (void**)&m_sampleGrabber);
	if (FAILED(hr)) return false;

	hr = m_graphBuilder->AddFilter(sampleGrabber, L"Grabber");
	if (FAILED(hr)) return false;
	
	// configure sample grabber
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB32;
	m_sampleGrabber->SetMediaType(&mt);
	m_sampleGrabber->SetBufferSamples(TRUE);
	m_sampleGrabber->SetOneShot(FALSE);

	// add black hole
	hr = m_nullRenderer.CoCreateInstance(CLSID_nullRenderer, NULL, CLSCTX_INPROC);
	if (FAILED(hr)) return false;
	hr = m_graphBuilder->AddFilter(m_nullRenderer, L"Black hole");
	if (FAILED(hr)) return false;

	ShowConfigDialog();
	
	hr = m_captureGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter, sampleGrabber, m_nullRenderer);	
	//hr = m_captureGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter, sampleGrabber, NULL);
	if (FAILED(hr)) return false;

	m_graphBuilder->QueryInterface(IID_IMediaControl, (void**)&m_mediaControl);
	if (FAILED(hr)) return false;

	AM_MEDIA_TYPE mt2;
	VIDEOINFOHEADER *pVih;
	hr = m_sampleGrabber->GetConnectedMediaType(&mt2);
	if (FAILED(hr)) return false;
	if (mt2.formattype == FORMAT_VideoInfo)
	{
		pVih = reinterpret_cast<VIDEOINFOHEADER*>(mt2.pbFormat);
		m_bmiHeader = pVih->bmiHeader;
	}
	MyFreeMediaType(mt2);

	hr = m_mediaControl->Run();
	if (FAILED(hr)) return false;
	
	return true;
}

void CamGrabber::Shutdown(void)
{
	if (m_mediaControl != NULL) m_mediaControl->Stop();
	m_mediaControl = NULL;
	m_sampleGrabber = NULL;
	m_nullRenderer = NULL;
	m_sourceFilter = NULL;
	m_graphBuilder = NULL;
	m_captureGraphBuilder = NULL;
}

int CamGrabber::SampleImage(unsigned char* bitmapdata, int size)
{
	long realSize;	
	HRESULT hr = m_sampleGrabber->GetCurrentBuffer(&realSize, NULL);
	if (FAILED(hr)) return 0;

	if (realSize > size) return 0;

	hr = m_sampleGrabber->GetCurrentBuffer(&realSize, (long*)bitmapdata);
	if (FAILED(hr)) return 0;

	return realSize;
}

BITMAPINFOHEADER CamGrabber::GetHeader(void)
{
	return m_bmiHeader;
}

void CamGrabber::ShowConfigDialog()
{
	IAMCrossbar* pX;

	// find crossbar
	HRESULT hr = m_captureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, 0, m_sourceFilter, IID_IAMStreamConfig, (void **)&pX);
	if (FAILED(hr)) return;

	CComPtr<ISpecifyPropertyPages> pSpec;
	CAUUID cauuid;

	hr = pX->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
	if (hr == S_OK)
	{
		hr = pSpec->GetPages(&cauuid);
		if (FAILED(hr)) return;
		hr = OleCreatePropertyFrame(m_hWnd, 30, 30, 0, 1, (IUnknown **)&pX, cauuid.cElems, (GUID *)cauuid.pElems, 0, 0, NULL);
		if (FAILED(hr)) return;

		CoTaskMemFree(cauuid.pElems);
	}
	pX->Release();
}

HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
	IMoniker * pMoniker = NULL;
	IRunningObjectTable *pROT = NULL;

	if (FAILED(GetRunningObjectTable(0, &pROT)))
	{
		return E_FAIL;
	}

	const size_t STRING_LENGTH = 256;

	WCHAR wsz[STRING_LENGTH];
	StringCchPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

	HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr))
	{
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,
			pMoniker, pdwRegister);
		pMoniker->Release();
	}
	pROT->Release();

	return hr;
}

void MyFreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// Unecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}
