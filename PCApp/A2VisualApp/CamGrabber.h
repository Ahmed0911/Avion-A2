#pragma once
#include <dshow.h>
#include <atlbase.h>

///////////////////////////////////////////////////////////////////////////////////

interface
	ISampleGrabberCB
	:
	public IUnknown
{
	virtual STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample) = 0;
	virtual STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) = 0;
};

///////////////////////////////////////////////////////////////////////////////////

static
const
IID IID_ISampleGrabberCB = { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };

///////////////////////////////////////////////////////////////////////////////////

interface
	ISampleGrabber
	:
	public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback, long WhichMethodToCallback) = 0;
};

///////////////////////////////////////////////////////////////////////////////////

static
const
IID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };

///////////////////////////////////////////////////////////////////////////////////

static
const
CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

///////////////////////////////////////////////////////////////////////////////////



class CamGrabber
{
public:
	CamGrabber();
	~CamGrabber();

public:
	bool Init(HWND hWnd);
	void Shutdown(void);
	void ShowConfigDialog();
	void Render();
	int SampleImage(unsigned char* bitmapdata, int size);
	BITMAPINFOHEADER GetHeader(void);

private:
	HWND m_hWnd;

	CComPtr<ICaptureGraphBuilder2>	m_captureGraphBuilder;
	CComPtr<IGraphBuilder>	m_graphBuilder;
	CComPtr<IBaseFilter>	m_sourceFilter;
	CComPtr<IBaseFilter>	m_nullRenderer;
	CComPtr<ISampleGrabber>	m_sampleGrabber;
	CComPtr<IMediaControl>	m_mediaControl;
	BITMAPINFOHEADER m_bmiHeader;
};

