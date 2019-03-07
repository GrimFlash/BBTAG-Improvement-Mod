#pragma once
#include "WindowManager.h"

#include <d3d9.h>

class OverlayManager
{
public:
	static OverlayManager& getInstance();
	bool Init(void *hwnd, IDirect3DDevice9 *device);
	void Update();
	void Render();
	void Shutdown();
	void InvalidateDeviceObjects();
	void CreateDeviceObjects();
	void OnMatchInit();
	bool IsInitialized() const;

	// start message with one of these: "[system]", "[info]", "[warning]", "[error]", "[fatal]", "[notice]", "[log]"
	void AddLog(const char* message, ...);
	void AddLogSeparator();
	void SetLogging(bool value);
	void SetNotification(const char *text, float timeToShowInSec = 5.0, bool showNotificationWindow = false);
	void SetUpdateAvailable();
private:
	OverlayManager() = default;
	void WriteLogToFile();
	void HandleButtons();

	static OverlayManager* m_instance;
	bool m_initialized = false;
	WindowManager* m_windowManager = nullptr;
};
