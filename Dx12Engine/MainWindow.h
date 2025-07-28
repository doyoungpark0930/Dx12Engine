#pragma once




class MainWindow : public BaseWindow<MainWindow>
{
public:
	ID3D11Device* m_Device;
	ID3D11DeviceContext* m_Context;
	IDXGISwapChain* m_SwapChain;
	ID3D11Texture2D* pBackBuffer;
	ID3D11RenderTargetView* m_renderTargetView;

	//depth stencil view와 proj의 aspect는 이거와 맞게 값 넣어줘야함
	int m_width = 1920;
	int m_height = 1050;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<ShaderSet> m_ShaderSet;

	float yaw = 0.0f;   // 좌우 회전 (Y축)
	float pitch = 0.0f; // 상하 회전 (X축)

	bool m_isMovingUp = false;
	bool m_isMovingDown = false;
	bool m_isMovingLeft = false;
	bool m_isMovingRight = false;

private:
	void CreateDevice();
	void DiscardGraphicsResources();
	void SetRtvAndBackBuffer();
	void Initialize();

public:
	PCWSTR ClassName() const override { return L"DoYeong's Engine"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void Update();
	void Render();

};

