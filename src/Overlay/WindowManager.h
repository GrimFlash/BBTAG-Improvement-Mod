#pragma once
#include "WindowDrawer.h"
#include "WindowContainer.h"

enum WindowType_
{
	WindowType_Main,
	WindowType_Debug,
	WindowType_Log,
	WindowType_Donators,
	WindowType_UpdateNotifier,
	WindowType_PaletteEditor,
	WindowType_CustomHud
};

class WindowManager : public WindowDrawer, public WindowContainer
{
public:
	WindowManager();
	~WindowManager() override = default;
	void DrawAllWindows();
protected:
	void FillWindowContainer() override;
private:
	ImVec2 m_scale;
};
