#include "OverlayManager.h"

#include "fonts.h"

#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/Settings.h"
#include "Core/utils.h"
#include "Web/update_check.h"
#include "Web/donators_fetch.h"
#include "Window/DebugWindow.h"
#include "Window/LogWindow.h"
#include "Window/PaletteEditorWindow.h"
#include <imgui.h>
#include <imgui_impl_dx9.h>

#include <sstream>
#include <time.h>

#define MAX_LOG_MSG_LEN 1024
#define MAIN_WINDOW_DISAPPEAR_TIME_SECONDS 15.0f
#define DEFAULT_ALPHA 0.87f

OverlayManager* OverlayManager::m_instance = nullptr;


bool *NO_CLOSE_FLAG = NULL;


float main_window_disappear_time = MAIN_WINDOW_DISAPPEAR_TIME_SECONDS;

std::string notificationText;
float notificationTimer = 0;

std::string main_title = "";

int toggle_key;
int toggleHUD_key;
int toggleCustomHUD_key;

ImVec2 middlescreen;

void OverlayManager::OnMatchInit()
{
	if (!m_initialized)
		return;

	g_interfaces.pPaletteManager->OnMatchInit(
		g_interfaces.player1.GetChar1(),
		g_interfaces.player1.GetChar2(),
		g_interfaces.player2.GetChar1(),
		g_interfaces.player2.GetChar2());

	m_mainWindow->getPaletteEditorWindow().OnMatchInit();
}

bool OverlayManager::IsInitialized() const
{
	return m_initialized;
}

void OverlayManager::SetMainWindowTitle(const char *text)
{
	if (text)
		main_title = text;
	else
	{
		main_title = MOD_WINDOW_TITLE;
		main_title += " ";
		main_title += MOD_VERSION_NUM;
#ifdef _DEBUG
		main_title += " (DEBUG)";
#endif
	}
	main_title += "###MainTitle"; //set unique identifier
}

OverlayManager & OverlayManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new OverlayManager();
	}

	return *m_instance;
}

bool OverlayManager::Init(void *hwnd, IDirect3DDevice9 *device)
{
	if (m_initialized)
		return true;

	LOG(2, "OverlayManager::Init\n");

	if (!hwnd)
	{
		LOG(2, "HWND not found!\n");
		return false;
	}
	if (!device)
	{
		LOG(2, "Direct3DDevice9 not found!\n");
		return false;
	}

	bool ret = ImGui_ImplDX9_Init(hwnd, device);
	if (!ret)
	{
		LOG(2, "ImGui_ImplDX9_Init failed!\n");
		return false;
	}

	SetMainWindowTitle();

	m_mainWindow = new MainWindow(MOD_WINDOW_TITLE, false, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowBorderSize = 1;
	style.FrameBorderSize = 1;
	style.ScrollbarSize = 14;
	style.Alpha = DEFAULT_ALPHA;

	//disable menu.ini saving?
	//ImGui::GetIO().IniFilename = NULL;

	if (Settings::settingsIni.menusize == 1)
	{
		ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(TinyFont_compressed_data_base85, 10);
		font->DisplayOffset.y += 1;
	}
	else if (Settings::settingsIni.menusize == 3)
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(DroidSans_compressed_data, DroidSans_compressed_size, 20);

	m_initialized = true;
	AddLog("[system] Initialization starting...\n");


	toggle_key = Settings::getButtonValue(Settings::settingsIni.togglebutton);
	AddLog("[system] Toggling key set to '%s'\n", Settings::settingsIni.togglebutton.c_str());

	toggleHUD_key = Settings::getButtonValue(Settings::settingsIni.toggleHUDbutton);
	AddLog("[system] HUD toggling key set to '%s'\n", Settings::settingsIni.toggleHUDbutton.c_str());

	toggleCustomHUD_key = Settings::getButtonValue(Settings::settingsIni.togglecustomHUDbutton);
	AddLog("[system] CustomHUD toggling key set to '%s'\n", Settings::settingsIni.togglecustomHUDbutton.c_str());

	g_interfaces.pPaletteManager->LoadAllPalettes();

	if (Settings::settingsIni.checkupdates)
	{
		HANDLE updateThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CheckUpdate, NULL, NULL, NULL);
		if(updateThread)
			CloseHandle(updateThread);
	}

	HANDLE donatorsThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)FetchDonators, NULL, NULL, NULL);
	if (donatorsThread)
		CloseHandle(donatorsThread);

	//Add the default font
	ImFontConfig cfg;
	cfg.SizePixels = 13;
	ImGui::GetIO().Fonts->AddFontDefault(&cfg);// ->DisplayOffset.y = SCALE;

	//Add our font
	ImFontConfig config;
	config.OversampleH = 3;
	config.OversampleV = 3;
	config.GlyphExtraSpacing.x = 1.0f;
	strcpy(config.Name, "CustomHUD");
	float hud_scale_y = ((float)Settings::settingsIni.renderheight * Settings::settingsIni.customhudscale) / 1042.0f;
	float fontsize = 30.0f;
	fontsize *= hud_scale_y;
	LOG(2, "CustomHUD fontsize: %f\n", fontsize);
	ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(TinyFont_compressed_data_base85, fontsize, &config);
	font->DisplayOffset.y += 1;

	/*Calling a frame to initialize beforehand to prevent a crash upon first call of
	/Update if the game window is not focused. Simply calling ImGui_ImplDX9_CreateDeviceObjects() might be enough too.*/
	ImGui_ImplDX9_NewFrame();
	ImGui::EndFrame();
	///////

	srand(time(NULL));

	AddLog("[system] Finished initialization\n");
	AddLogSeparator();

	LOG(2, "Init end\n");
	return ret;
}

void OverlayManager::Shutdown()
{
	if (!m_initialized)
		return;

	LOG(2, "OverlayManager::Shutdown\n");
	WriteLogToFile();

	SAFE_DELETE(m_mainWindow);
	delete m_instance;

	ImGui_ImplDX9_Shutdown();
}

void OverlayManager::InvalidateDeviceObjects()
{
	if (!m_initialized)
		return;

	LOG(2, "OverlayManager::InvalidateDeviceObjects\n");
	ImGui_ImplDX9_InvalidateDeviceObjects();
}

void OverlayManager::CreateDeviceObjects()
{
	if (!m_initialized)
		return;

	LOG(2, "OverlayManager::CreateDeviceObjects\n");
	ImGui_ImplDX9_CreateDeviceObjects();
}

void OverlayManager::Render()
{
	if (!m_initialized)
		return;

	LOG(7, "OverlayManager::Render\n");
	ImGui::Render();
}

void OverlayManager::Update()
{
	if (!m_initialized)
		return;

	LOG(7, "OverlayManager::Update\n");

	g_interfaces.pPaletteManager->OnUpdate(
		g_interfaces.player1.GetChar1().GetPalHandle(),
		g_interfaces.player1.GetChar2().GetPalHandle(),
		g_interfaces.player2.GetChar1().GetPalHandle(),
		g_interfaces.player2.GetChar2().GetPalHandle());

	HandleMainWindowVisibility(main_window_disappear_time);

	// return if game window is minimized, to avoid the custom hud elements
	// being thrown in the upper left corner due to resolution shrinking
	if (IsIconic(g_gameProc.hWndGameWindow))
		return;

	HandleButtons();

	ImGui_ImplDX9_NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	bool isUpdateNotifierWindowOpen = m_mainWindow->getUpdateNotifierWindow().IsOpen();
	bool isPaletteEditorWindowOpen = m_mainWindow->getPaletteEditorWindow().IsOpen();
	bool isLogWindowOpen = m_mainWindow->getLogWindow().IsOpen();

	io.MouseDrawCursor = m_mainWindow->IsOpen() | isLogWindowOpen
		| isPaletteEditorWindowOpen | isUpdateNotifierWindowOpen; // show_notification_window | show_demo_window;

	if (Settings::settingsIni.viewportoverride == VIEWPORT_OVERRIDE)
	{
		io.DisplaySize = ImVec2((float)Settings::settingsIni.renderwidth, (float)Settings::settingsIni.renderheight);
	}

	ShowAllWindows();

	LOG(7, "END OF OverlayManager::Update\n");
}

void OverlayManager::SetUpdateAvailable()
{
	m_mainWindow->getUpdateNotifierWindow().Open();
}

// start with type a of message: "[system]", "[info]", "[warning]", "[error]", "[fatal]", "[notice]", "[log]"
void OverlayManager::AddLog(const char* message, ...)
{
	if (!m_initialized || !m_mainWindow->getLogWindow().IsLoggingOn())
	{ 
		return; 
	}

	//Get current time into a string
	time_t current_time;
	struct tm * time_info;
	char timeString[9];  // extra space for nullbyte: "HH:MM:SS\0"

	time(&current_time);
	time_info = localtime(&current_time);

	strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);

	if (strlen(message) > MAX_LOG_MSG_LEN)
	{
		LOG(2, "AddLog error: message too long!\nmessage: %s", message);
		m_mainWindow->getLogWindow().AddLog("%s [error] Log message too long.", timeString);
		return;
	}

	std::string fullMessage(timeString);

	char buf[MAX_LOG_MSG_LEN];
	va_list args;
	va_start(args, message);
	vsprintf(buf, message, args);
	va_end(args);

	fullMessage += " ";
	fullMessage += buf;

	m_mainWindow->getLogWindow().AddLog(fullMessage.c_str());
}

void OverlayManager::AddLogSeparator()
{
	m_mainWindow->getLogWindow().AddLog("------------------------------------------------------------------\n");
}

void OverlayManager::SetLogging(bool value)
{
	m_mainWindow->getLogWindow().SetLogging(value);
}

void OverlayManager::WriteLogToFile()
{
	if (!m_initialized)
		return;

	FILE *file = fopen("BBTAG_IM\\log.txt", "a");

	if (!file)
	{
		LOG(2, "OverlayManager::WriteLogToFile file opening failed!!\n");
		return;
	}

	char* time = getFullDate();

	if (time)
	{
		fprintf(file, "BBTAGIM %s -- %s\n", MOD_VERSION_NUM, time);
		free(time);
	}
	else
	{
		fprintf(file, "{Couldn't get the current time}\n");
	}

	fprintf(file, "-------------------------------------\n\n");
	fprintf(file, "settings.ini config:\n");

	//TODO: Put the strings into the X-Macro as well. Somehow...
	//strings cant be put into the X-Macro (.c_str() cannot be put on non-std::string types)
	fprintf(file, "\t- ToggleButton: %s\n", Settings::settingsIni.togglebutton.c_str());
	fprintf(file, "\t- ToggleHUDButton: %s\n", Settings::settingsIni.toggleHUDbutton.c_str());
	fprintf(file, "\t- ToggleCustomHUDButton: %s\n", Settings::settingsIni.togglecustomHUDbutton.c_str());

	std::string printText = "";

	//X-Macro
#define SETTING(_type, _var, _inistring, _defaultval) \
	printText = "\t- "; \
	printText += _inistring; \
	if(strcmp(#_type, "bool") == 0 || strcmp(#_type, "int") == 0) \
		printText += ": %d\n"; \
	else if(strcmp(#_type, "float") == 0) \
		printText += ": %.2f\n"; \
	if(strcmp(#_type, "std::string") != 0) \
		fprintf(file, printText.c_str(), Settings::settingsIni.##_var);
#include "Core/settings.def"
#undef SETTING

	fprintf(file, "\n");

	//d3dparams here

	m_mainWindow->getLogWindow().ToFile(file);
	fprintf(file, "\n#####################################\n\n\n");

	fclose(file);
}


void OverlayManager::HandleMainWindowVisibility(float timeLeft)
{
	main_window_disappear_time = timeLeft;

	//start making the mod's menu disappear upon start, within MAIN_WINDOW_DISAPPEAR_TIME_SECONDS
	if (main_window_disappear_time > 0)
	{	
		main_window_disappear_time -= ImGui::GetIO().DeltaTime;
		
		if(main_window_disappear_time > 0)
			ImGui::GetStyle().Alpha = main_window_disappear_time / MAIN_WINDOW_DISAPPEAR_TIME_SECONDS;

		//disappear upon reaching main_window_disappear_time < 0
		if (main_window_disappear_time <= 0)
		{
			m_mainWindow->Close();
			ImGui::GetStyle().Alpha = DEFAULT_ALPHA;
		}
	}
	else
	{
		ImGui::GetStyle().Alpha = DEFAULT_ALPHA;
	}
}

void OverlayManager::ShowAllWindows()
{
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, DEFAULT_ALPHA);

	m_mainWindow->Update();
	m_mainWindow->UpdateWindows();

////////////// DEBUG Windows
#ifdef _DEBUG
	//if (show_demo_window)
	//{
	//	ImGui::SetNextWindowPos(ImVec2(550, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
	//	ImGui::ShowDemoWindow(&show_demo_window);
	//}
#endif
////////////// 

	ImGui::PopStyleVar();
}

void OverlayManager::HandleButtons()
{
	if (!m_initialized)
	{
		return;
	}

	if (ImGui::IsKeyPressed(toggleHUD_key) && g_gameVals.pIsHUDHidden)
	{
		*g_gameVals.pIsHUDHidden ^= 1;
	}

	if (ImGui::IsKeyPressed(toggleCustomHUD_key))
	{
		// show_custom_hud ^= 1;
	}

	if (ImGui::IsKeyPressed(toggle_key))
	{
		m_mainWindow->ToggleOpen();
		HandleMainWindowVisibility(0);
	}
}
