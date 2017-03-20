#include "stdafx.h"
#include "Dir2D.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")

#define SCREENX 1920
#define SCREENY 1080
//#define VFOV 35
#define VFOV 41.7

#define DRAWHORIZON 1
#define NOVIDEO 0

CDir2D::CDir2D(void)
{
}


CDir2D::~CDir2D(void)
{
}

void CDir2D::Init(HWND hWnd, TCHAR* mapName)
{	
	// create D2D1 object
	D2D1_FACTORY_OPTIONS options;
	//options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	DX::ThrowIfFailed(
		D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&m_d2dFactory )
	);

    // You need it for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
    // This array defines the set of DirectX hardware feature levels this app  supports.
    // The ordering is important and you should  preserve it.
    // Don't forget to declare your app's minimum required feature level in its
    // description.  All apps are assumed to support 9.1 unless otherwise stated.
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

	// Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
	
	DX::ThrowIfFailed(
        D3D11CreateDevice(
            nullptr,                    // specify null to use the default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            0,                          
            creationFlags,              // optionally set debug and Direct2D compatibility flags
            featureLevels,              // list of feature levels this app can support
            ARRAYSIZE(featureLevels),   // number of possible feature levels
            D3D11_SDK_VERSION,          
            &device,                    // returns the Direct3D device created
            &m_featureLevel,            // returns feature level of device created
            &context                    // returns the device immediate context
            )
        );

    ComPtr<IDXGIDevice2> dxgiDevice;
    // Obtain the underlying DXGI device of the Direct3D11 device.
    DX::ThrowIfFailed(
        device.As(&dxgiDevice)
        );

    // Obtain the Direct2D device for 2-D rendering.
    DX::ThrowIfFailed(
        m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
        );

    // Get Direct2D device's corresponding device context object.
    DX::ThrowIfFailed(
        m_d2dDevice->CreateDeviceContext( D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext )
        );

	// Allocate a descriptor.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
	swapChainDesc.Width = SCREENX;                           // use automatic sizing
	swapChainDesc.Height = SCREENY;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
    swapChainDesc.Stereo = false; 
    swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
    swapChainDesc.Flags = 0;

    // Identify the physical adapter (GPU or card) this device is runs on.
    ComPtr<IDXGIAdapter> dxgiAdapter;
    DX::ThrowIfFailed(
        dxgiDevice->GetAdapter(&dxgiAdapter)
        );

    // Get the factory object that created the DXGI device.
    ComPtr<IDXGIFactory2> dxgiFactory;
    DX::ThrowIfFailed(
        dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
        );

    // Get the final swap chain for this window from the DXGI factory.
    DX::ThrowIfFailed(
		dxgiFactory->CreateSwapChainForHwnd(
            device.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,    // allow on all displays
			nullptr,
            &m_swapChain
            )
        );

    // Ensure that DXGI doesn't queue more than one frame at a time.    
	DX::ThrowIfFailed(
        dxgiDevice->SetMaximumFrameLatency(1)
        );

    // Get the backbuffer for this window which is be the final 3D render target.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        );


	// Now we set up the Direct2D render target bitmap linked to the swapchain. 
    // Whenever we render to this bitmap, it will be directly rendered to the 
    // swapchain associated with the window.
	m_dpi = 96;
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1( D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), m_dpi, m_dpi );

    // Direct2D needs the dxgi version of the backbuffer surface pointer.
    ComPtr<IDXGISurface> dxgiBackBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
        );

    // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
    DX::ThrowIfFailed(
        m_d2dContext->CreateBitmapFromDxgiSurface(
            dxgiBackBuffer.Get(),
            &bitmapProperties,
            &m_d2dTargetBitmap
            )
        );

    // So now we can set the Direct2D render target.
    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

	CreateUserResources();

#if !NOVIDEO
	// create cam grabber
	if (m_camGrabber.Init(hWnd))
	{
		// create bitmap
		BITMAPINFOHEADER bihdr = m_camGrabber.GetHeader();
		int size = bihdr.biSizeImage;
		unsigned char* buff = new unsigned char[size];
		D2D1_BITMAP_PROPERTIES1 prop;
		prop.bitmapOptions = D2D1_BITMAP_OPTIONS_NONE;
		prop.colorContext = nullptr;
		prop.dpiX = m_dpi;
		prop.dpiY = m_dpi;
		prop.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		m_d2dContext->CreateBitmap(D2D1::SizeU(bihdr.biWidth, bihdr.biHeight), buff, bihdr.biBitCount / 8 * bihdr.biWidth, &prop, &m_CamVideoBitmap);
	}
#endif
	// create WiC Encoder
	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory) )
		);

	// create bitmap for encoding
	D2D1_BITMAP_PROPERTIES1 bitmapPropertiesSave = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), m_dpi, m_dpi);

	// Create local bitmap for savoToBitmap
	D2D1_SIZE_U pixelSize = m_d2dContext->GetPixelSize();
	DX::ThrowIfFailed(
		m_d2dContext->CreateBitmap(
			pixelSize,
			nullptr,
			pixelSize.width * 4,    // pitch = width * size of pixel (4 bytes for B8G8R8A8)
			&bitmapPropertiesSave,
			&m_saveBitmap)
		);	

	// create map
	m_Map.LoadMap(mapName, m_d2dContext.Get(), m_DWriteFactory.Get(), hWnd);

	// Init Parameters
	m_Parameters.Init(m_d2dContext.Get(), m_DWriteFactory.Get(), hWnd);

	m_ActiveDisplay = EActiveDisplay::CAMERA; // TODO: ADD TRANSFORMATION
}

void CDir2D::CreateUserResources(void)
{	
	// Create a DirectWrite factory.
	DX::ThrowIfFailed(
		DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(m_DWriteFactory.GetAddressOf()) )
	);

	// real stuff
	DX::ThrowIfFailed(
		m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow), &m_GreenBrush)
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CadetBlue), &m_BlueBrush)
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::OrangeRed), &m_RedBrush)
		);
	

	// gradient brush (red/yellow/green)	
	D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Red, 1);
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Yellow, 1);
	gradientStops[1].position = 0.3f;
	gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::Green, 1);
	gradientStops[2].position = 1.0f;
	ComPtr<ID2D1GradientStopCollection> pGradientStops;
	DX::ThrowIfFailed( 
		m_d2dContext->CreateGradientStopCollection(
			gradientStops,
			3,
			D2D1_GAMMA_2_2,
			D2D1_EXTEND_MODE_CLAMP,
			&pGradientStops)
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(
			D2D1::Point2F(0, 0),
			D2D1::Point2F(150, 0)),
			pGradientStops.Get(),
			&m_pLinearGradientRYGBrush)
		);


	D2D1_STROKE_STYLE_PROPERTIES properties = D2D1::StrokeStyleProperties();
	properties.dashStyle = D2D1_DASH_STYLE_CUSTOM;
	float dashes[] = { 8.0f, 6.0f };

	DX::ThrowIfFailed(
		m_d2dFactory->CreateStrokeStyle(properties,
		dashes,
		ARRAYSIZE(dashes),
		&m_DashedLineStroke)
	);

	// create font
	// Create main Green font
	DX::ThrowIfFailed(
		m_DWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24,
		L"", //locale
		&m_GreenTextFormat)
		);	
	m_GreenTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_GreenTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// Create large Green font
	DX::ThrowIfFailed(
		m_DWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		48,
		L"", //locale
		&m_GreenLargeTextFormat)
		);
	m_GreenLargeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_GreenLargeTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// Create large Red Warning font
	DX::ThrowIfFailed(
		m_DWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		60,
		L"", //locale
		&m_RedLargeWaringTextFormat)
		);
	m_RedLargeWaringTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_RedLargeWaringTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void CDir2D::DrawVideo()
{
	// draw cam image (Bitmap style)
	if (m_CamVideoBitmap != NULL)
	{
		if (m_ActiveDisplay == EActiveDisplay::CAMERA)
		{
			m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
		}
		else
		{
			m_d2dContext->SetTransform(D2D1::Matrix3x2F::Scale(0.14F, 0.2F) * D2D1::Matrix3x2F::Translation(SCREENX * 0.01F, SCREENY * 0.78F));
		}
	

		BITMAPINFOHEADER bihdr = m_camGrabber.GetHeader();
		int size = bihdr.biSizeImage;
		unsigned char* buff = new unsigned char[size];
		int sz = m_camGrabber.SampleImage(buff, size);
		m_CamVideoBitmap->CopyFromMemory(&D2D1::RectU(0, 0, bihdr.biWidth, bihdr.biHeight), buff, bihdr.biBitCount / 8 * bihdr.biWidth);
		delete[] buff;
		D2D1::Matrix4x4F matFlip;
		matFlip._22 = -1; // flip Y axis (-SCREENY)
		// FIX ratio (square pixels)
		float sizeY = SCREENY;
		//float sizeX = SCREENY * 4.0 / 3.0;
		float sizeX = SCREENY * 16.0 / 9.0;
		float sideCorrection = (SCREENX - sizeX) / 2;
		m_d2dContext->DrawBitmap(m_CamVideoBitmap.Get(), &D2D1::RectF(sideCorrection, 0, SCREENX - sideCorrection, -SCREENY), 1, D2D1_INTERPOLATION_MODE_ANISOTROPIC, 0, &matFlip);

		// draw cam helper (box)
		//m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY / 2));
		//m_d2dContext->DrawRectangle(&D2D1::RectF(-300, -300, 300, 300), m_BlueBrush.Get(), 5);
	}
}

void CDir2D::DrawHUD(SUserData &data, bool noTelemetry)
{
	// get data
	float pixPerDeg = (float)(SCREENY / VFOV);
	float pitch = data.Pitch;
	float roll = data.Roll;
	float yaw = data.Yaw;
	float speed = data.Speed;
	float altitude = data.Altitude;
	float vertspeed = data.Vertspeed;
	int motorthrusts[4];
	for (int i = 0; i != 4; i++)
	{
		motorthrusts[i] = data.MotorThrusts[i];
	}
	int txRSSI = data.RXA2RSSI;
	int rxRSSI = data.RXControlStationRSSI;
	float fuelLevel = data.FuelLevel;

	// fix crap
	if (data.HorizontalAccuracy > 1e6) data.HorizontalAccuracy = 0;
	if (data.VerticalAccuracy > 1e6) data.VerticalAccuracy = 0;


	// debug
	static double x = 0;
	x = x + 0.005;
	//pitch = 20;
	//roll = 45;
	//yaw = (float)(sin(x + 1.2) * 50 + 50);
	/*speed = (float)(sin(x) * 15 + 15);
	altitude = (float)(sin(x) * 40 + 50);
	vertspeed = (float)(sin(x) * 9.5);
	motorthrusts[0] = (int)(sin(x) * 50 + 50);
	motorthrusts[1] = (int)(sin(x + 0.5) * 50 + 50);
	motorthrusts[2] = (int)(sin(x + 1.2) * 50 + 50);
	motorthrusts[3] = (int)(sin(x - 2.2) * 50 + 50);
	txRSSI = (int)(sin(x) * 20 - 60);
	rxRSSI = (int)(sin(x-2.2) * 20 - 60);
	fuelLevel = (float)(sin(x + 0.5) * 50 + 50);*/

	// draw only message on NO_TELEMETRY
	if (noTelemetry)
	{
		// draw at center
		m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY *0.1f));
		DrawNumber(DNSTYLE_RED_WARNING, -250, 0, 500, 50, 0, 0, L"NO TELEMETRY");

		return;
	}

	// GPS/MAP
	// TEST
	//data.Latitude =  (int)( (45.797469 + (0.005*sin(x)))*1e7 );
	//data.Longitude = (int)( (15.889805 + (0.003*sin(1.8*x)))*1e7);
	//data.NumSV = 12;
	
	// draw map
	if (m_ActiveDisplay == EActiveDisplay::CAMERA)
	{
		m_d2dContext->SetTransform(D2D1::Matrix3x2F::Scale(0.38F, 0.38F) * D2D1::Matrix3x2F::Translation(SCREENX * 0.006F, SCREENY * 0.78F));
	}
	else if(m_ActiveDisplay == EActiveDisplay::MAP )
	{
		m_d2dContext->SetTransform(D2D1::Matrix3x2F::Scale(1.7, 1.7) * D2D1::Matrix3x2F::Translation(SCREENX * 0.13F, SCREENY * 0.03F));
	}
	m_Map.Draw(m_d2dContext.Get(), data.Longitude*1e-7, data.Latitude*1e-7);


	// Draw Parameters
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY / 2));
	m_Parameters.Draw(m_d2dContext.Get());


	// draw at center
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY / 2));
	// draw nose marker	
	m_d2dContext->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(0, 0), 20, 20), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-50, 0), D2D1::Point2F(-20, 0), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(50, 0), D2D1::Point2F(20, 0), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(0, -50), D2D1::Point2F(0, -20), m_GreenBrush.Get(), 2, NULL);

	// draw horizon	
#if DRAWHORIZON
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(0, pitch * pixPerDeg) * D2D1::Matrix3x2F::Rotation(-roll) * D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY / 2));
	m_d2dContext->DrawLine(D2D1::Point2F(-550, 0), D2D1::Point2F(-100, 0), m_GreenBrush.Get(), 4, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(550, 0), D2D1::Point2F(100, 0), m_GreenBrush.Get(), 4, NULL);

	for (float i = 10; i <= 90; i += 10)
	{
		m_d2dContext->DrawLine(D2D1::Point2F(-200, -i*pixPerDeg), D2D1::Point2F(-60, -i*pixPerDeg), m_GreenBrush.Get(), 2, NULL);
		m_d2dContext->DrawLine(D2D1::Point2F(200, -i*pixPerDeg), D2D1::Point2F(60, -i*pixPerDeg), m_GreenBrush.Get(), 2, NULL);
		m_d2dContext->DrawLine(D2D1::Point2F(-200, -i*pixPerDeg), D2D1::Point2F(-200, -i*pixPerDeg + 20), m_GreenBrush.Get(), 2, NULL);
		m_d2dContext->DrawLine(D2D1::Point2F(200, -i*pixPerDeg), D2D1::Point2F(200, -i*pixPerDeg + 20), m_GreenBrush.Get(), 2, NULL);

		// text
		DrawNumber(DNSTYLE_SMALL, -250, -i*pixPerDeg, 50, 20, (int)i);
		DrawNumber(DNSTYLE_SMALL, 200, -i*pixPerDeg, 50, 20, (int)i);
	}

	for (float i = -90; i <= -10; i += 10)
	{
		m_d2dContext->DrawLine(D2D1::Point2F(-200, -i*pixPerDeg), D2D1::Point2F(-60, -i*pixPerDeg), m_GreenBrush.Get(), 2, m_DashedLineStroke.Get());
		m_d2dContext->DrawLine(D2D1::Point2F(200, -i*pixPerDeg), D2D1::Point2F(60, -i*pixPerDeg), m_GreenBrush.Get(), 2, m_DashedLineStroke.Get());
		m_d2dContext->DrawLine(D2D1::Point2F(-200, -i*pixPerDeg), D2D1::Point2F(-200, -i*pixPerDeg - 20), m_GreenBrush.Get(), 2, m_DashedLineStroke.Get());
		m_d2dContext->DrawLine(D2D1::Point2F(200, -i*pixPerDeg), D2D1::Point2F(200, -i*pixPerDeg - 20), m_GreenBrush.Get(), 2, m_DashedLineStroke.Get());

		// text
		DrawNumber(DNSTYLE_SMALL, -250, -i*pixPerDeg - 18, 50, 20, (int)i);
		DrawNumber(DNSTYLE_SMALL, 200, -i*pixPerDeg - 18, 50, 20, (int)i);
	}
	
	// draw roll/banking
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY *0.7)); // translate to bottom mid
	for (int i = -60; i <= 60; i += 10)
	{
		float dx = (float)sin(i / 180.0*3.1415);
		float dy = (float)cos(i / 180.0*3.1415);
		if ((i % 20) == 0)
		{
			m_d2dContext->DrawLine(D2D1::Point2F(dx * 200, dy * 200), D2D1::Point2F(dx * 230, dy * 230), m_GreenBrush.Get(), 2, NULL);
		}
		else
		{
			m_d2dContext->DrawLine(D2D1::Point2F(dx * 215, dy * 215), D2D1::Point2F(dx * 230, dy * 230), m_GreenBrush.Get(), 1, NULL);
		}
	}
	float bank = -roll;
	if (bank > 65) bank = 65;
	if (bank < -65) bank = -65;
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(0, 190) * D2D1::Matrix3x2F::Rotation(bank) * D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY *0.7));
	m_d2dContext->DrawLine(D2D1::Point2F(-20, -30), D2D1::Point2F(0, 0), m_GreenBrush.Get(), 1, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(20, -30), D2D1::Point2F(0, 0), m_GreenBrush.Get(), 1, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-20, -30), D2D1::Point2F(20, -30), m_GreenBrush.Get(), 1, NULL);
#endif

	// draw yaw/heading
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX / 2, SCREENY *0.05)); // translate to top mid
	m_d2dContext->DrawLine(D2D1::Point2F(0, -40), D2D1::Point2F(0, 0), m_GreenBrush.Get(), 2, NULL);
	if (yaw < 0) yaw += 360;
	DrawNumber(DNSTYLE_YAW_BOX, -50, 50, 100, 50, (int)yaw);
	m_d2dContext->DrawRectangle(D2D1::RectF(-50, 50, 50, 100), m_GreenBrush.Get(), 2);
	int firstRoundedYaw = ((int)yaw / 10) * 10;
	float off = (yaw - firstRoundedYaw);
	for (int i = -60; i != 70; i += 10)
	{
		float x = i - off; // shift for yaw
		x = x * 10;
		int yawToDraw = (int)firstRoundedYaw + i;
		if (yawToDraw < 0) yawToDraw += 360;
		if (yawToDraw >= 360) yawToDraw -= 360;
		if ((yawToDraw % 20) == 0)
		{
			m_d2dContext->DrawLine(D2D1::Point2F(x, 0), D2D1::Point2F(x, 30), m_GreenBrush.Get(), 2, NULL);
			DrawNumber(DNSTYLE_SMALL, x - 25, 30, 50, 30, yawToDraw);
		}
		else
		{
			m_d2dContext->DrawLine(D2D1::Point2F(x, 0), D2D1::Point2F(x, 15), m_GreenBrush.Get(), 1, NULL);
		}
	}
	
	// speed indicator
	int MaxSpeed = 100; // km/h
	float SpeedOffset = 600.0f / MaxSpeed;
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.05F, SCREENY * 0.47F));
	for (int i = 0; i <= MaxSpeed; i += 2)
	{
		if ((i % 10) == 0)
		{
			m_d2dContext->DrawLine(D2D1::Point2F(0, (i - MaxSpeed / 2) * SpeedOffset), D2D1::Point2F(40, (i - MaxSpeed / 2) * SpeedOffset), m_GreenBrush.Get(), 2, NULL);
			DrawNumber(DNSTYLE_SMALL, -50, (MaxSpeed / 2 - i) * SpeedOffset - 15.0f, 50, 30, i);
		}
		else
		{
			m_d2dContext->DrawLine(D2D1::Point2F(20, (i - MaxSpeed / 2) * SpeedOffset), D2D1::Point2F(40, (i - MaxSpeed / 2) * SpeedOffset), m_GreenBrush.Get(), 1, NULL);
		}
	}
	m_d2dContext->DrawLine(D2D1::Point2F(40, 300), D2D1::Point2F(40, -300), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.05 + 45, SCREENY *0.47F + (MaxSpeed / 2 - speed) * SpeedOffset));
	m_d2dContext->DrawLine(D2D1::Point2F(0, 0), D2D1::Point2F(25, -25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(25, -25), D2D1::Point2F(120, -25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(0, 0), D2D1::Point2F(25, 25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(25, 25), D2D1::Point2F(120, 25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(120, -25), D2D1::Point2F(120, 25), m_GreenBrush.Get(), 2, NULL);
	DrawNumber(DNSTYLE_YAW_BOX_FLOAT, 20, -25, 100, 50, 0, speed);
	

	// altitude
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.87F, SCREENY / 2));
	float altD = (altitude - 50);
	int roundedAltD = ((int)altD / 10) * 10;
	float roundedAltOffsetD = roundedAltD - altD;
	float altDPointerOffset = 0;
	if (altD <= 0)
	{
		roundedAltD = 0;
		roundedAltOffsetD = 0;
		altDPointerOffset = -altD * 7;

		if (altDPointerOffset > 400) altDPointerOffset = 400;
	}
	for (int i = 0; i <= 120; i += 2)
	{
		if (((50 - i - roundedAltOffsetD) * 7.0f) < 380 && ((50 - i - roundedAltOffsetD) * 7.0f) > -380)
		{
			if ((i % 10) == 0)
			{
				m_d2dContext->DrawLine(D2D1::Point2F(0, (50 - i - roundedAltOffsetD) * 7.0f), D2D1::Point2F(40, (50 - i - roundedAltOffsetD) * 7.0f), m_GreenBrush.Get(), 1, NULL);
				DrawNumber(DNSTYLE_SMALL, 40, (50 - i - roundedAltOffsetD) * 7.0f - 15, 60, 30, i + roundedAltD);
			}
			else
			{
				m_d2dContext->DrawLine(D2D1::Point2F(0, (50 - i - roundedAltOffsetD) * 7.0f), D2D1::Point2F(20, (50 - i - roundedAltOffsetD) * 7.0f), m_GreenBrush.Get(), 1, NULL);
			}
		}
	}

	m_d2dContext->DrawLine(D2D1::Point2F(0, 400), D2D1::Point2F(0, -400), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(0, 400), D2D1::Point2F(40, 400), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(0, -400), D2D1::Point2F(40, -400), m_GreenBrush.Get(), 2, NULL);

	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.87F - 3, SCREENY / 2 + altDPointerOffset));
	m_d2dContext->DrawLine(D2D1::Point2F(0, 0), D2D1::Point2F(-25, -25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-25, -25), D2D1::Point2F(-150, -25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(0, 0), D2D1::Point2F(-25, 25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-25, 25), D2D1::Point2F(-150, 25), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-150, -25), D2D1::Point2F(-150, 25), m_GreenBrush.Get(), 2, NULL);
	DrawNumber(DNSTYLE_YAW_BOX, -150, -25, 125, 50, (int)altitude);

	// vert speed
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.95, SCREENY / 2));
	m_d2dContext->DrawLine(D2D1::Point2F(0, 220), D2D1::Point2F(0, -220), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-10, 0), D2D1::Point2F(35, 0), m_GreenBrush.Get(), 3, NULL);
	for (int i = -10; i <= 10; i += 1)
	{
		if (i % 2 == 0)
		{
			m_d2dContext->DrawLine(D2D1::Point2F(0, i * 20.0F), D2D1::Point2F(30, i * 20.0F), m_GreenBrush.Get(), 2, NULL);
			DrawNumber(DNSTYLE_SMALL, 25, (float)i * 20 - 16, 50, 30, -i);
		}
		else
		{
			m_d2dContext->DrawLine(D2D1::Point2F(0, i * 20.0F), D2D1::Point2F(20, i * 20.0F), m_GreenBrush.Get(), 1, NULL);
		}
	}
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.95, SCREENY / 2 - vertspeed * 20));
	m_d2dContext->DrawLine(D2D1::Point2F(-20, -20), D2D1::Point2F(0, 0), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-20, 20), D2D1::Point2F(0, 0), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-20, 20), D2D1::Point2F(-20, -20), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.95, SCREENY / 2 + 180));
	DrawNumber(DNSTYLE_YAW_BOX_FLOAT, -50, 50, 100, 50, 0, vertspeed);
	m_d2dContext->DrawRectangle(D2D1::RectF(-50, 50, 50, 100), m_GreenBrush.Get(), 2);

	// draw engine thrusts
	for (int i = 0; i != 4; i++)
	{
		m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.015F + i * 55, SCREENY * 0.05F));
		m_d2dContext->FillRectangle(D2D1::RectF(0, 100.0F - motorthrusts[i], 20, 100), m_BlueBrush.Get());
		m_d2dContext->DrawRectangle(D2D1::RectF(0, 0, 20, 100), m_GreenBrush.Get(), 2);
		TCHAR buf[10];
		if (i == 0) swprintf_s(buf, 10, L"THR");
		else if (i == 1) swprintf_s(buf, 10, L"AIL");
		else if (i == 2) swprintf_s(buf, 10, L"ELE");
		else if (i == 3) swprintf_s(buf, 10, L"RUD");
		DrawNumber(DNSTYLE_SMALL, -15, 110, 52, 20, 0, 0, buf);
		DrawNumber(DNSTYLE_SMALL, -15, -25, 50, 20, motorthrusts[i]);
	}

	// draw Comm signal
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.87F, SCREENY * 0.04F));
	float percentRX = (100.0F + rxRSSI) / (100 - 36);
	if (percentRX > 1) percentRX = 1;
	if (percentRX < 0) percentRX = 0;
	m_d2dContext->FillRectangle(D2D1::RectF(0, 0, percentRX * 150, 20), m_pLinearGradientRYGBrush.Get());
	m_d2dContext->DrawRectangle(D2D1::RectF(0, 0, 150, 20), m_GreenBrush.Get(), 2);
	DrawNumber(DNSTYLE_SMALL, -40, 0, 40, 20, 0, 0, L"RX");
	TCHAR buf[200];
	swprintf_s(buf, 20, L"%d dB", rxRSSI);
	DrawNumber(DNSTYLE_SMALL, 150, 0, 80, 20, 0, 0, buf);

	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.87F, SCREENY * 0.07F));
	float percentTX = (100.0F + txRSSI) / (100 - 36);
	if (percentTX > 1) percentTX = 1;
	if (percentTX < 0) percentTX = 0;
	m_d2dContext->FillRectangle(D2D1::RectF(0, 0, percentTX * 150, 20), m_pLinearGradientRYGBrush.Get());
	m_d2dContext->DrawRectangle(D2D1::RectF(0, 0, 150, 20), m_GreenBrush.Get(), 2);
	DrawNumber(DNSTYLE_SMALL, -40, 0, 40, 20, 0, 0, L"TX");
	swprintf_s(buf, 20, L"%d dB", txRSSI);
	DrawNumber(DNSTYLE_SMALL, 150, 0, 80, 20, 0, 0, buf);

	// draw battery indicator
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.80F, SCREENY * 0.92F));
	if (fuelLevel < 0) fuelLevel = 0;
	if (fuelLevel > 100) fuelLevel = 100;
	m_d2dContext->FillRectangle(D2D1::RectF(0, 0, fuelLevel/100 * 300, 40), m_pLinearGradientRYGBrush.Get());
	m_d2dContext->DrawRectangle(D2D1::RectF(0, 0, 300, 40), m_GreenBrush.Get(), 2);
	swprintf_s(buf, 20, L"%d%%", (int)fuelLevel);
	DrawNumber(DNSTYLE_YAW_BOX, 90, -5, 140, 50, 0, 0, buf);
	m_d2dContext->DrawRectangle(D2D1::RectF(-50, 10, -10, 35), m_GreenBrush.Get(), 2);
	m_d2dContext->DrawLine(D2D1::Point2F(-50, 10), D2D1::Point2F(-40, 0), m_GreenBrush.Get(), 2, NULL);
	m_d2dContext->DrawLine(D2D1::Point2F(-10, 10), D2D1::Point2F(-20, 0), m_GreenBrush.Get(), 2, NULL);
	DrawNumber(DNSTYLE_SMALL, -47, 13, 20, 20, 0, 0, L"+");
	DrawNumber(DNSTYLE_SMALL, -30, 11, 20, 20, 0, 0, L"-");
	swprintf_s(buf, 20, L"V: %0.1f V", data.BatteryVoltage);
	DrawNumber(DNSTYLE_SMALL,-10, -30, 100, 20, 0, 0, buf);
	swprintf_s(buf, 20, L"C: %0.1f A", data.BatteryCurrentA);
	DrawNumber(DNSTYLE_SMALL, -10+100, -30, 130, 20, 0, 0, buf);
	swprintf_s(buf, 20, L"Q: %0.0f mAh", data.BatteryTotalCharge_mAh);
	DrawNumber(DNSTYLE_SMALL, -10 + 230, -30, 150, 20, 0, 0, buf);

	// GPS
	float Yoffset = 5;
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.165F, SCREENY * 0.78F));
	m_GreenTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	swprintf_s(buf, 200, L"GPS Time: %0.2f", data.GPSTime / 1000.0f);
	//DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"Sat Num#: %d", data.NumSV);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset, 180, 25, 0, 0, buf);
	// fix type
	std::wstring fixTypes[] = { L"[]", L"[2D]", L"[3D]", L"[OK]", L"[Time]" };
	std::wstring fixtyp = L"";
	if (data.FixType <= 5) fixtyp += fixTypes[data.FixType];
	DrawNumber(DNSTYLE_SMALL, 150, 25 * Yoffset++, 100, 25, 0, 0, (TCHAR*)fixtyp.c_str());
	// fix flags
	std::wstring fixStr = L"NONE ";
	if (data.FixFlags & 0x01) fixStr += L"FIX ";
	if (data.FixFlags & 0x02) fixStr += L"DIFF ";
	if (data.FixFlags & 0x20) fixStr += L"HDG ";
	std::wstring psmState[] = { L"", L"ENABLED", L"ACQUISITION", L"TRACKING", L"PWROPT", L"INACTIVE", L"X", L"X", L"X" };
	int psmIndex = (data.FixFlags >> 3) && 0x07;
	fixStr += psmState[(psmIndex)];
	//DrawNumber(DNSTYLE_SMALL, 0, 25*Yoffset++, 350, 25, 0, 0, (TCHAR*)fixStr.c_str());
	// speeds
	swprintf_s(buf, 200, L"N:%0.1f m/s E:%0.1f m/s", data.VelN / 1000.0, data.VelE / 1000.0);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"Vert: %0.1f m/s [%0.1f m/s]", data.VelD / 1000.0, data.SpeedAcc / 1000.0);
	//DrawNumber(DNSTYLE_SMALL, 0, 25*Yoffset++, 350, 25, 0, 0, buf);
	// acc
	swprintf_s(buf, 200, L"AccH: %0.2f m AccV: %0.2f m", data.HorizontalAccuracy/1000.0, data.VerticalAccuracy/1000.0f);
	//DrawNumber(DNSTYLE_SMALL, 0, 25*Yoffset++, 450, 25, 0, 0, buf);
	// position
	swprintf_s(buf, 200, L"Lat: %0.5f Long: %0.5f ", data.Latitude*1e-7, data.Longitude*1e-7 );
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"MSL: %0.3f m", data.HeightMSL/1000.0);
	//DrawNumber(DNSTYLE_SMALL, 0, 25*Yoffset++, 350, 25, 0, 0, buf);
	if (m_Map.HaveHome())
	{
		double distanceFromHome = m_Map.DistanceFromHomeMeters(data.Longitude*1e-7, data.Latitude*1e-7);
		swprintf_s(buf, 200, L"Home: %0.1f m", distanceFromHome);
		DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	}
	// GPS AutoFIX
	if (!m_Map.HaveHome())
	{
		// no home yet
		if (data.NumSV >= 6) // 6 or more sats
		{
			m_Map.SetHome(data.Longitude*1e-7, data.Latitude*1e-7, data.HeightMSL);
		}
	}


	// System data
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.58F, SCREENY * 0.97F));
	swprintf_s(buf, 20, L"LoopCnt: %d", data.LoopCounter);
	DrawNumber(DNSTYLE_SMALL, 0, 0, 200, 20, 0, 0, buf);
	swprintf_s(buf, 20, L"CS_Frm: %d", data.RXControlStationFrameCount);
	DrawNumber(DNSTYLE_SMALL, 200, 0, 200, 20, 0, 0, buf);
	swprintf_s(buf, 20, L"A2_Frm: %d (%d)", data.RXA2RSSIFrameCount, data.RXA2RSSI);
	DrawNumber(DNSTYLE_SMALL, 370, 0, 300, 20, 0, 0, buf);	
	swprintf_s(buf, 20, L"%0.1lf s", data.LocalTime);
	DrawNumber(DNSTYLE_SMALL, 600, 0, 200, 20, 0, 0, buf);
	
	// Comm Data
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.165F, SCREENY * 0.78F));
	m_GreenTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	Yoffset = 0;
	swprintf_s(buf, 200, L"MsgReceivedOK: %d", data.MsgReceivedOK);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"CRC Errors: %d", data.CrcErrors);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"HeaderFails: %d", data.HeaderFails);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);
	swprintf_s(buf, 200, L"TimeoutCounter: %d", data.TimeoutCounter);
	DrawNumber(DNSTYLE_SMALL, 0, 25 * Yoffset++, 350, 25, 0, 0, buf);

	// Mode
	m_d2dContext->SetTransform(D2D1::Matrix3x2F::Translation(SCREENX * 0.015F, SCREENY * 0.005F));
	if (data.ActualMode >= 0 && data.ActualMode <= 5)
	{
		std::wstring modes[] = { L"Off", L"Manual", L"Attitude", L"Arcade", L"Mode4", L"Mode5" };
		swprintf_s(buf, 100, L"Mode: %s [%d]", modes[data.ActualMode].c_str(), data.ActualMode);
		m_Map.m_ActiveWaypoint = -1;
	}
	else
	{
		swprintf_s(buf, 100, L"Mode: WayPnt [%d]", data.ActualMode-10);
		m_Map.m_ActiveWaypoint = data.ActualMode - 10;
	}
	DrawNumber(DNSTYLE_SMALL, 0, 0, 300, 20, 0, 0, buf);
	
	m_GreenTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER); // restore alignment
}

void CDir2D::DrawNumber(int style, float x, float y, float dx, float dy, int number, float numberF, TCHAR* text)
{
	if (style == DNSTYLE_SMALL)
	{
		TCHAR buffer[100];
		if (text != NULL) wcscpy_s(buffer, text);
		else _itow_s(number, buffer, 100, 10);
		D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x+dx, y+dy);				
		m_d2dContext->DrawText(
			buffer,        // The string to render.
			(UINT32)_tcslen(buffer),    // The string's length.
			m_GreenTextFormat.Get(),    // The text format.
			&layoutRect,       // The region of the window where the text will be rendered.
			m_GreenBrush.Get()     // The brush used to draw the text.
			);

		//m_d2dContext->DrawRectangle(&layoutRect, m_GreenBrush.Get()); // DEBUG
	}
	else if (style == DNSTYLE_YAW_BOX)
	{
		TCHAR buffer[100];
		if (text != NULL) wcscpy_s(buffer, text);
		else _itow_s(number, buffer, 100, 10);
		D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + dx, y + dy);
		m_d2dContext->DrawText(
			buffer,        // The string to render.
			(UINT32)_tcslen(buffer),    // The string's length.
			m_GreenLargeTextFormat.Get(),    // The text format.
			&layoutRect,       // The region of the window where the text will be rendered.
			m_GreenBrush.Get()     // The brush used to draw the text.
			);

		//m_d2dContext->DrawRectangle(&layoutRect, m_GreenBrush.Get());
	}
	else if (style == DNSTYLE_YAW_BOX_FLOAT)
	{
		TCHAR buffer[100];
		swprintf_s(buffer, 100, L"%0.1f", numberF);
		D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + dx, y + dy);
		m_d2dContext->DrawText(
			buffer,        // The string to render.
			(UINT32)_tcslen(buffer),    // The string's length.
			m_GreenLargeTextFormat.Get(),    // The text format.
			&layoutRect,       // The region of the window where the text will be rendered.
			m_GreenBrush.Get()     // The brush used to draw the text.
			);

		//m_d2dContext->DrawRectangle(&layoutRect, m_GreenBrush.Get());
	}
	else if (style == DNSTYLE_RED_WARNING)
	{
		TCHAR buffer[100];
		if (text != NULL) wcscpy_s(buffer, text);
		else _itow_s(number, buffer, 100, 10);
		D2D1_RECT_F layoutRect = D2D1::RectF(x, y, x + dx, y + dy);
		m_d2dContext->DrawText(
			buffer,        // The string to render.
			(UINT32)_tcslen(buffer),    // The string's length.
			m_RedLargeWaringTextFormat.Get(),    // The text format.
			&layoutRect,       // The region of the window where the text will be rendered.
			m_RedBrush.Get()     // The brush used to draw the text.
			);
		//m_d2dContext->DrawRectangle(&layoutRect, m_RedBrush.Get());
	}
}

void CDir2D::Draw(SUserData &data, bool noTelemetry)
{
	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get()); // TODO: profile this call, remove if necessary
	m_d2dContext->BeginDraw();
	m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

	// draw Video
	DrawVideo();

	// draw HUD
	DrawHUD(data, noTelemetry);


	DX::ThrowIfFailed(
		m_d2dContext->EndDraw()
	);
	
	DXGI_PRESENT_PARAMETERS parameters = {0};
	DX::ThrowIfFailed(
		m_swapChain->Present1(1, 0, &parameters)
	);

}

void CDir2D::DrawToBitmap(SUserData &data, std::wstring bitmapfile, bool noTelemetry)
{
	m_d2dContext->SetTarget(m_saveBitmap.Get());
	m_d2dContext->BeginDraw();
	m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.0f)); // transparent background

	// draw HUD
	DrawHUD(data, noTelemetry);

	m_d2dContext->EndDraw();

	// store to file
	// Create and initialize WIC Bitmap Encoder.	
	ComPtr<IWICBitmapEncoder> wicBitmapEncoder;	
	GUID wicFormat = GUID_ContainerFormatPng;
	DX::ThrowIfFailed(
		m_wicFactory->CreateEncoder(
		wicFormat,
		nullptr,    // No preferred codec vendor.
		&wicBitmapEncoder)
		);

	ComPtr<IStream> stream;
	SHCreateStreamOnFile(bitmapfile.c_str(), STGM_CREATE | STGM_WRITE, &stream);
	DX::ThrowIfFailed(
		wicBitmapEncoder->Initialize(
		stream.Get(),
		WICBitmapEncoderNoCache)
		);

	// Create and initialize WIC Frame Encoder.
	ComPtr<IWICBitmapFrameEncode> wicFrameEncode;
	DX::ThrowIfFailed(
		wicBitmapEncoder->CreateNewFrame(
		&wicFrameEncode,
		nullptr)
		);

	DX::ThrowIfFailed(
		wicFrameEncode->Initialize(nullptr)
		);

	// Create IWICImageEncoder.
	ComPtr<IWICImageEncoder> imageEncoder;
	DX::ThrowIfFailed(
		m_wicFactory->CreateImageEncoder(
		m_d2dDevice.Get(),
		&imageEncoder)
		);

	DX::ThrowIfFailed(
		imageEncoder->WriteFrame(
		m_saveBitmap.Get(),
		//m_d2dTargetBitmap.Get(),
		wicFrameEncode.Get(),
		nullptr)
		);

	DX::ThrowIfFailed(
		wicFrameEncode->Commit()
		);

	DX::ThrowIfFailed(
		wicBitmapEncoder->Commit()
		);

	// Flush all memory buffers to the next-level storage object.
	DX::ThrowIfFailed(
		stream->Commit(STGC_DEFAULT)
		);
}

CMapManager* CDir2D::GetMapMgr()
{
	return &m_Map;
}

void CDir2D::Shutdown(void)
{
	m_d2dTargetBitmap = nullptr;
	m_swapChain = nullptr;
	m_d2dContext = nullptr;
	m_d2dDevice = nullptr;
	m_d2dFactory = nullptr;
	m_saveBitmap = nullptr;
	m_wicFactory = nullptr;
}
