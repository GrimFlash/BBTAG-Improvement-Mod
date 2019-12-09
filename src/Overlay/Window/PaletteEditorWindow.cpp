#include "PaletteEditorWindow.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/Logger/ImGuiLogger.h"
#include "PaletteManager/impl_format.h"

#include <imgui.h>

#include <Shlwapi.h>

#define NUMBER_OF_COLOR_BOXES (IMPL_PALETTE_DATALEN / sizeof(int)) // 256
#define COLUMNS 16
const int COLOR_BLACK = 0xFF000000;
const int COLOR_WHITE = 0xFFFFFFFF;
const ImVec4 COLOR_DONATOR(1.000f, 0.794f, 0.000f, 1.000f);
const ImVec4 COLOR_ONLINE(0.260f, 0.590f, 0.980f, 1.000f);

static char palNameBuf[IMPL_PALNAME_LENGTH] = "";
static char palDescBuf[IMPL_DESC_LENGTH] = "";
static char palCreatorBuf[IMPL_CREATOR_LENGTH] = "";

void PaletteEditorWindow::ShowAllPaletteSelections()
{
	if (HasNullPointer())
	{
		return;
	}

	ShowPaletteSelectButton(g_interfaces.player1.GetChar1(), "P1Ch1 palette", "select1-1");
	ShowPaletteSelectButton(g_interfaces.player1.GetChar2(), "P1Ch2 palette", "select1-2");
	ShowPaletteSelectButton(g_interfaces.player2.GetChar1(), "P2Ch1 palette", "select2-1");
	ShowPaletteSelectButton(g_interfaces.player2.GetChar2(), "P2Ch2 palette", "select2-2");
}

void PaletteEditorWindow::ShowReloadAllPalettesButton()
{
	if (ImGui::Button("Reload custom palettes"))
	{
		g_interfaces.pPaletteManager->ReloadAllPalettes();
	}
}

void PaletteEditorWindow::OnMatchInit()
{
	if (HasNullPointer())
	{
		return;
	}

	InitializeSelectedCharacters();

	m_selectedCharIndex = (CharIndex)m_allSelectedCharHandles[0]->GetData()->char_index;
	m_selectedCharName = m_allSelectedCharNames[0].c_str();
	m_selectedCharPalHandle = &m_allSelectedCharHandles[0]->GetPalHandle();
	m_selectedPalIndex = g_interfaces.pPaletteManager->GetCurrentCustomPalIndex(*m_selectedCharPalHandle);
	CopyPalTextsToTextBoxes(*m_selectedCharPalHandle);
	m_selectedFile = PaletteFile_Character;

	m_colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha;
	m_highlightMode = false;
	m_showAlpha = false;

	CopyPalFileToEditorArray(m_selectedFile, *m_selectedCharPalHandle);
}

void PaletteEditorWindow::Draw()
{
	if (!isPaletteEditingEnabledInCurrentGameMode() || HasNullPointer())
	{
		Close();
		return;
	}

	CheckSelectedPalOutOfBound();

	CharacterSelection();
	PaletteSelection();
	FileSelection();
	EditingModesSelection();
	ShowPaletteBoxes();
	SavePaletteToFile();
}

bool PaletteEditorWindow::HasNullPointer()
{
	if (g_interfaces.player1.GetChar1().IsCharDataNullPtr() ||
		g_interfaces.player1.GetChar2().IsCharDataNullPtr() ||
		g_interfaces.player2.GetChar1().IsCharDataNullPtr() ||
		g_interfaces.player2.GetChar2().IsCharDataNullPtr())
	{
		return true;
	}

	return false;
}

void PaletteEditorWindow::InitializeSelectedCharacters()
{
	m_allSelectedCharHandles[0] = &g_interfaces.player1.GetChar1();
	m_allSelectedCharHandles[1] = &g_interfaces.player1.GetChar2();
	m_allSelectedCharHandles[2] = &g_interfaces.player2.GetChar1();
	m_allSelectedCharHandles[3] = &g_interfaces.player2.GetChar2();

	m_allSelectedCharNames[0] = getCharacterNameByIndexA(m_allSelectedCharHandles[0]->GetData()->char_index);
	m_allSelectedCharNames[1] = getCharacterNameByIndexA(m_allSelectedCharHandles[1]->GetData()->char_index);
	m_allSelectedCharNames[2] = getCharacterNameByIndexA(m_allSelectedCharHandles[2]->GetData()->char_index);
	m_allSelectedCharNames[3] = getCharacterNameByIndexA(m_allSelectedCharHandles[3]->GetData()->char_index);
}

void PaletteEditorWindow::CharacterSelection()
{
	LOG(7, "PaletteEditorWindow CharacterSelection\n");

	if (ImGui::Button("Select character"))
	{
		ImGui::OpenPopup("select_char_pal");
	}
	ImGui::SameLine();

	ImGui::Text(m_selectedCharName);

	if (ImGui::BeginPopup("select_char_pal"))
	{
		const int NUMBER_OF_CHARS = 4;
		
		for (int i = 0; i < NUMBER_OF_CHARS; i++)
		{
			ImGui::PushID(i);
			if (ImGui::Selectable(m_allSelectedCharNames[i].c_str()))
			{
				DisableHighlightModes();

				m_selectedCharIndex = (CharIndex)m_allSelectedCharHandles[i]->GetData()->char_index;
				m_selectedCharName = m_allSelectedCharNames[i].c_str();
				m_selectedCharPalHandle = &m_allSelectedCharHandles[i]->GetPalHandle();
				m_selectedPalIndex = g_interfaces.pPaletteManager->GetCurrentCustomPalIndex(*m_selectedCharPalHandle);
				CopyPalFileToEditorArray(m_selectedFile, *m_selectedCharPalHandle);
			}
			ImGui::PopID();
		}
		ImGui::EndPopup();
	}
}

void PaletteEditorWindow::PaletteSelection()
{
	LOG(7, "PaletteEditorWindow PaletteSelection\n");

	if (ImGui::Button("Select palette  "))
	{
		ImGui::OpenPopup("select_custom_pal");
	}

	ImGui::SameLine();
	ImGui::Text(m_customPaletteVector[m_selectedCharIndex][m_selectedPalIndex].palname);

	ShowPaletteSelectPopup(*m_selectedCharPalHandle, m_selectedCharIndex, "select_custom_pal");
}

void PaletteEditorWindow::FileSelection()
{
	LOG(7, "PaletteEditorWindow FileSelection\n");

	if (ImGui::Button("Select file     "))
	{
		ImGui::OpenPopup("select_file_pal");
	}
	ImGui::SameLine();

	ImGui::Text(palFileNames[m_selectedFile]);

	if (ImGui::BeginPopup("select_file_pal"))
	{
		for (int i = 0; i < TOTAL_PALETTE_FILES; i++)
		{
			if (ImGui::Selectable(palFileNames[i]))
			{
				DisableHighlightModes();
				m_selectedFile = (PaletteFile)(i);
				CopyPalFileToEditorArray(m_selectedFile, *m_selectedCharPalHandle);
			}
		}
		ImGui::EndPopup();
	}
}

void PaletteEditorWindow::EditingModesSelection()
{
	LOG(7, "PaletteEditorWindow EditingModesSelection\n");

	ImGui::Separator();
	if (ImGui::Checkbox("Show transparency values", &m_showAlpha))
	{
		m_colorEditFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha;
		if (m_showAlpha)
		{
			m_colorEditFlags &= ~ImGuiColorEditFlags_NoAlpha;
			m_colorEditFlags |= ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar;
		}
	}

	ImGui::SameLine();
	int nextLineColumnPosX = ImGui::GetCursorPosX();
	ImGui::Checkbox("Freeze frame", &g_gameVals.isFrameFrozen);
	
	if (ImGui::Checkbox("Highlight mode", &m_highlightMode))
	{
		if (m_highlightMode)
		{
			//fill the array with black
			for (int i = 0; i < NUMBER_OF_COLOR_BOXES; i++)
			{
				((int*)m_highlightArray)[i] = COLOR_BLACK;
			}
			g_interfaces.pPaletteManager->ReplacePaletteFile(m_highlightArray, m_selectedFile, *m_selectedCharPalHandle);
		}
		else
		{
			DisableHighlightModes();
		}
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(nextLineColumnPosX);
	ImGui::Checkbox("Show indexes", &m_showIndexes);

	if (ImGui::Button("Gradient generator"))
	{
		ImGui::OpenPopup("gradient");
	}
	ShowGradientPopup();

	ImGui::Separator();
}

void PaletteEditorWindow::ShowPaletteBoxes()
{
	LOG(7, "PaletteEditorWindow ShowPaletteBoxes\n");

	ImGui::Text("");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

	if (m_showIndexes)
	{
		ImGui::TextUnformatted("001 "); ImGui::SameLine();
	}

	for (int i = 0, col = 1; i < NUMBER_OF_COLOR_BOXES; i++)
	{
		ImGui::PushID(i);

		bool pressed = false;
		int curColorBoxOffset = (i * sizeof(int));

		if (m_highlightMode)
		{
			pressed = ImGui::ColorButtonOn32Bit("##PalColorButton", (unsigned char*)m_paletteEditorArray + curColorBoxOffset, m_colorEditFlags);
		}
		else
		{
			pressed = ImGui::ColorEdit4On32Bit("##PalColorEdit", (unsigned char*)m_paletteEditorArray + curColorBoxOffset, m_colorEditFlags);
		}

		if (pressed)
		{
			if (m_highlightMode)
			{
				UpdateHighlightArray(i);
			}
			else
			{
				g_interfaces.pPaletteManager->ReplacePaletteFile(m_paletteEditorArray, m_selectedFile, *m_selectedCharPalHandle);
			}
		}

		if (col < COLUMNS)
		{
			//continue the row
			ImGui::SameLine();
			col++;
		}
		else
		{
			//start a new row
			col = 1;
			if (m_showIndexes && i < NUMBER_OF_COLOR_BOXES - 1)
			{
				ImGui::Text("%.3d ", i + 2);
				ImGui::SameLine();
			}
		}
		ImGui::PopID();
	}
	ImGui::PopStyleVar();
}

void PaletteEditorWindow::DisableHighlightModes()
{
	m_highlightMode = false;
	g_interfaces.pPaletteManager->ReplacePaletteFile(m_paletteEditorArray, m_selectedFile, *m_selectedCharPalHandle);
}

void PaletteEditorWindow::SavePaletteToFile()
{
	static char message[200] = "";

	ImGui::Text("");
	ImGui::Separator();

	if (m_highlightMode)
	{
		ImGui::TextDisabled("Cannot save with Highlight mode on!");
		return;
	}

	struct TextFilters { static int FilterAllowedChars(ImGuiTextEditCallbackData* data) { if (data->EventChar < 256 && strchr(" qwertzuiopasdfghjklyxcvbnmQWERTZUIOPASDFGHJKLYXCVBNM0123456789_.()[]!@&+-'^,;{}$=", (char)data->EventChar)) return 0; return 1; } };

	ImGui::Text("Palette name:");
	ImGui::PushItemWidth(250);
	ImGui::InputText("##palName", palNameBuf, IMPL_PALNAME_LENGTH, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAllowedChars);
	ImGui::PopItemWidth();

	ImGui::Text("Creator (optional):");
	ImGui::PushItemWidth(250);
	ImGui::InputText("##palcreator", palCreatorBuf, IMPL_CREATOR_LENGTH, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAllowedChars);
	ImGui::PopItemWidth();

	ImGui::Text("Palette description (optional):");
	ImGui::PushItemWidth(250);
	ImGui::InputText("##palDesc", palDescBuf, IMPL_DESC_LENGTH, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterAllowedChars);
	ImGui::PopItemWidth();

	bool pressed = ImGui::Button("Save palette");
	ImGui::Text(message);

	static bool show_overwrite_popup = false;

	if (!pressed && !show_overwrite_popup)
	{
		return;
	}

	if (strncmp(palNameBuf, "", IMPL_PALNAME_LENGTH) == 0)
	{
		memcpy(message, "Error, no filename given", 25);
		g_imGuiLogger->Log("[error] Could not save custom palette, no filename was given\n");
		return;
	}

	if (strncmp(palNameBuf, "Default", IMPL_PALNAME_LENGTH) == 0 || strncmp(palNameBuf, "Random", IMPL_PALNAME_LENGTH) == 0)
	{
		memcpy(message, "Error, not a valid filename", 28);
		g_imGuiLogger->Log("[error] Could not save custom palette: not a valid filename\n");
		return;
	}

	TCHAR pathBuf[MAX_PATH];
	GetModuleFileName(NULL, pathBuf, MAX_PATH);
	std::wstring::size_type pos = std::wstring(pathBuf).find_last_of(L"\\");
	std::wstring wFullPath = std::wstring(pathBuf).substr(0, pos);

	wFullPath += L"\\BBTAG_IM\\Palettes\\";
	wFullPath += getCharacterNameByIndexW(m_selectedCharIndex);
	wFullPath += L"\\";

	std::string filenameTemp(palNameBuf);
	std::wstring wFilename(filenameTemp.begin(), filenameTemp.end());
	wFullPath += wFilename;

	if (wFilename.find(L".impl") == std::wstring::npos)
	{
		wFullPath += L".impl";
		filenameTemp += ".impl";
	}

	if (ShowOverwritePopup(&show_overwrite_popup, wFullPath.c_str(), filenameTemp.c_str()))
	{

		IMPL_data_t curPalData = g_interfaces.pPaletteManager->GetCurrentPalData(*m_selectedCharPalHandle);

		strncpy(curPalData.creator, palCreatorBuf, IMPL_CREATOR_LENGTH);
		strncpy(curPalData.palname, palNameBuf, IMPL_PALNAME_LENGTH);
		strncpy(curPalData.desc, palDescBuf, IMPL_DESC_LENGTH);

		std::string messageText = "'";
		messageText += filenameTemp.c_str();

		if (g_interfaces.pPaletteManager->WritePaletteToFile(m_selectedCharIndex, &curPalData))
		{
			std::string fullPath(wFullPath.begin(), wFullPath.end());
			g_imGuiLogger->Log("[system] Custom palette '%s' successfully saved to:\n'%s'\n", filenameTemp.c_str(), fullPath.c_str());
			messageText += "' saved successfully";

			ReloadSavedPalette(palNameBuf);
		}
		else
		{
			g_imGuiLogger->Log("[error] Custom palette '%s' failed to be saved.\n", filenameTemp.c_str());
			messageText += "' save failed";
		}

		memcpy(message, messageText.c_str(), messageText.length() + 1);
	}
}

void PaletteEditorWindow::ReloadSavedPalette(const char* palName)
{
	g_imGuiLogger->EnableLog(false);
	g_interfaces.pPaletteManager->ReloadAllPalettes();
	g_imGuiLogger->EnableLog(true);

	//find the newly loaded custom pal
	m_selectedPalIndex = g_interfaces.pPaletteManager->FindCustomPalIndex(m_selectedCharIndex, palName);

	if (m_selectedPalIndex < 0)
	{
		g_imGuiLogger->Log("[error] Saved custom palette couldn't be reloaded. Not found.\n");
		m_selectedPalIndex = 0;
	}

	g_interfaces.pPaletteManager->SwitchPalette(m_selectedCharIndex, *m_selectedCharPalHandle, m_selectedPalIndex);
	CopyPalFileToEditorArray(m_selectedFile, *m_selectedCharPalHandle);
}

bool PaletteEditorWindow::ShowOverwritePopup(bool *p_open, const wchar_t* wFullPath, const char* filename)
{
	bool isOverwriteAllowed = true;

	if (PathFileExists(wFullPath))
	{
		ImGui::OpenPopup("Overwrite?");
		*p_open = true;
	}
	if (ImGui::BeginPopupModal("Overwrite?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("'%s' already exists.\nAre you sure you want to overwrite it?\n\n", filename);
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			*p_open = false;
			isOverwriteAllowed = true;
			return isOverwriteAllowed;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			*p_open = false;
		}
		ImGui::EndPopup();

		isOverwriteAllowed = false;
	}

	return isOverwriteAllowed;
}

void PaletteEditorWindow::CheckSelectedPalOutOfBound()
{
	if (m_selectedPalIndex != 0 && m_selectedPalIndex >= m_customPaletteVector[m_selectedCharIndex].size())
	{
		//reset back to default
		m_selectedPalIndex = 0;
		g_interfaces.pPaletteManager->SwitchPalette(m_selectedCharIndex, *m_selectedCharPalHandle, m_selectedPalIndex);
		CopyPalFileToEditorArray(m_selectedFile, *m_selectedCharPalHandle);
	}
}

void PaletteEditorWindow::ShowPaletteSelectButton(CharHandle & charHandle, const char * btnText, const char * popupID)
{
	CharPaletteHandle& charPalHandle = charHandle.GetPalHandle();
	int selected_pal_index = g_interfaces.pPaletteManager->GetCurrentCustomPalIndex(charPalHandle);
	CharIndex charIndex = (CharIndex)charHandle.GetData()->char_index;

	if (charIndex >= getCharactersCountWithoutBoss())
	{
		return;
	}

	ShowPaletteRandomizerButton(popupID, charHandle);
	ImGui::SameLine();

	if (ImGui::Button(btnText))
	{
		ImGui::OpenPopup(popupID);
	}
	HoverTooltip(getCharacterNameByIndexA(charHandle.GetData()->char_index).c_str());

	ImGui::SameLine();
	ImGui::TextUnformatted(m_customPaletteVector[charIndex][selected_pal_index].palname);

	ShowPaletteSelectPopup(charPalHandle, charIndex, popupID);
}

void PaletteEditorWindow::ShowPaletteSelectPopup(CharPaletteHandle& charPalHandle, CharIndex charIndex, const char* popupID)
{
	static int hoveredPalIndex = 0;
	bool pressed = false;
	int onlinePalsStartIndex = g_interfaces.pPaletteManager->GetOnlinePalsStartIndex(charIndex);
	ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, 25.0f), ImVec2(-1.0f, 300.0f));

	if (ImGui::BeginPopup(popupID))
	{
		ImGui::TextUnformatted(getCharacterNameByIndexA(charIndex).c_str());
		ImGui::Separator();
		for (int i = 0; i < m_customPaletteVector[charIndex].size(); i++)
		{
			if (i == onlinePalsStartIndex)
			{
				ImGui::PushStyleColor(ImGuiCol_Separator, COLOR_ONLINE);
				ImGui::Separator();
				ImGui::PopStyleColor();
			}

			if (ImGui::Selectable(m_customPaletteVector[charIndex][i].palname))
			{
				pressed = true;
				g_interfaces.pPaletteManager->SwitchPalette(charIndex, charPalHandle, i);

				//updating palette editor's array if this is the currently selected character
				if (&charPalHandle == m_selectedCharPalHandle)
				{
					m_selectedPalIndex = i;
					CopyPalFileToEditorArray(m_selectedFile, charPalHandle);
					DisableHighlightModes();

					CopyPalTextsToTextBoxes(charPalHandle);
				}
			}
			if (ImGui::IsItemHovered())
			{
				hoveredPalIndex = i;
			}
			ShowHoveredPaletteToolTip(charPalHandle, charIndex, i);
		}
		ImGui::EndPopup();
	}
	HandleHoveredPaletteSelection(&charPalHandle, charIndex, hoveredPalIndex, popupID, pressed);
}

void PaletteEditorWindow::ShowHoveredPaletteToolTip(CharPaletteHandle& charPalHandle, CharIndex charIndex, int palIndex)
{
	if (!ImGui::IsItemHovered())
	{
		return;
	}

	const char* creatorText = m_customPaletteVector[charIndex][palIndex].creator;
	const char* descText = m_customPaletteVector[charIndex][palIndex].desc;
	const int creatorLen = strnlen(creatorText, IMPL_CREATOR_LENGTH);
	const int descLen = strnlen(descText, IMPL_DESC_LENGTH);
	bool isOnlinePal = palIndex >= g_interfaces.pPaletteManager->GetOnlinePalsStartIndex(charIndex);

	if (creatorLen || descLen || isOnlinePal)
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(300.0f);

		if (isOnlinePal)
			ImGui::TextColored(COLOR_ONLINE, "ONLINE PALETTE");
		if (creatorLen)
			ImGui::Text("Creator: %s", creatorText);
		if (descLen)
			ImGui::Text("Description: %s", descText);

		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void PaletteEditorWindow::HandleHoveredPaletteSelection(CharPaletteHandle* charPalHandle, CharIndex charIndex, int palIndex, const char* popupID, bool pressed)
{
	static CharPaletteHandle* prevCharHndl = 0;
	static int prevPalIndex = 0;
	static int origPalIndex = 0;
	static bool paletteSwitched = false;
	static char popupIDbkp[32];
	const char* palFileAddr = 0;

	if (pressed)
	{
		paletteSwitched = false;
	}
	else if (!ImGui::IsPopupOpen(popupID) && strcmp(popupIDbkp, popupID) == 0 
		&& paletteSwitched && prevCharHndl == charPalHandle && !pressed)
	{
		palFileAddr = g_interfaces.pPaletteManager->GetCustomPalFile(charIndex, origPalIndex, PaletteFile_Character, *charPalHandle);
		g_interfaces.pPaletteManager->ReplacePaletteFile(palFileAddr, PaletteFile_Character, *charPalHandle);
		paletteSwitched = false;
	}
	else if (ImGui::IsPopupOpen(popupID) && prevPalIndex != palIndex)
	{
		if (!paletteSwitched)
		{
			origPalIndex = g_interfaces.pPaletteManager->GetCurrentCustomPalIndex(*charPalHandle);
		}

		palFileAddr = g_interfaces.pPaletteManager->GetCustomPalFile(charIndex, palIndex, PaletteFile_Character, *charPalHandle);
		g_interfaces.pPaletteManager->ReplacePaletteFile(palFileAddr, PaletteFile_Character, *charPalHandle);
		prevPalIndex = palIndex;
		prevCharHndl = charPalHandle;
		paletteSwitched = true;
		strcpy(popupIDbkp, popupID);
	}
}

void PaletteEditorWindow::ShowPaletteRandomizerButton(const char * btnID, CharHandle& charHandle)
{
	int charIndex = charHandle.GetData()->char_index;
	char buf[12];
	sprintf(buf, "?##%s", btnID);
	
	if (ImGui::Button(buf) && m_customPaletteVector[charIndex].size() > 1)
	{
		CharPaletteHandle& charPalHandle = charHandle.GetPalHandle();
		int curPalIndex = g_interfaces.pPaletteManager->GetCurrentCustomPalIndex(charPalHandle);
		int newPalIndex = curPalIndex;

		while (curPalIndex == newPalIndex)
		{
			newPalIndex = rand() % m_customPaletteVector[charIndex].size();
		}
		g_interfaces.pPaletteManager->SwitchPalette((CharIndex)charIndex, charPalHandle, newPalIndex);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Random selection");
		ImGui::EndTooltip();
	}
}

void PaletteEditorWindow::CopyToEditorArray(const char * pSrc)
{
	memcpy(m_paletteEditorArray, pSrc, IMPL_PALETTE_DATALEN);
}

void PaletteEditorWindow::CopyPalFileToEditorArray(PaletteFile palFile, CharPaletteHandle & charPalHandle)
{
	const char* fileAddr = g_interfaces.pPaletteManager->GetPalFileAddr(palFile, charPalHandle);
	CopyToEditorArray(fileAddr);
}

void PaletteEditorWindow::UpdateHighlightArray(int selectedBoxIndex)
{
	static int selected_highlight_box = 0;

	//setting previously pressed box back to black
	((int*)m_highlightArray)[selected_highlight_box] = COLOR_BLACK;

	selected_highlight_box = selectedBoxIndex;

	//setting currently pressed box to white
	((int*)m_highlightArray)[selected_highlight_box] = COLOR_WHITE;

	g_interfaces.pPaletteManager->ReplacePaletteFile(m_highlightArray, m_selectedFile, *m_selectedCharPalHandle);
}

void PaletteEditorWindow::CopyPalTextsToTextBoxes(CharPaletteHandle & charPalHandle)
{
	IMPL_data_t palData = g_interfaces.pPaletteManager->GetCurrentPalData(charPalHandle);
	strncpy(palNameBuf, palData.palname, IMPL_PALNAME_LENGTH);
	strncpy(palDescBuf, palData.desc, IMPL_DESC_LENGTH);
	strncpy(palCreatorBuf, palData.creator, IMPL_CREATOR_LENGTH);
}

void PaletteEditorWindow::ShowGradientPopup()
{
	if (ImGui::BeginPopup("gradient"))
	{
		ImGui::TextUnformatted("Gradient generator");

		static int idx1 = 1;
		static int idx2 = 2;
		int minVal_idx2 = idx1 + 1;

		if (idx2 <= idx1)
		{
			idx2 = minVal_idx2;
		}

		ImGui::SliderInt("Start index", &idx1, 1, NUMBER_OF_COLOR_BOXES - 1);
		ImGui::SliderInt("End index", &idx2, minVal_idx2, NUMBER_OF_COLOR_BOXES);

		static int color1 = 0xFFFFFFFF;
		static int color2 = 0xFFFFFFFF;
		int alpha_flag = m_colorEditFlags & ImGuiColorEditFlags_NoAlpha;

		ImGui::ColorEdit4On32Bit("Start color", (unsigned char*)&color1, alpha_flag);
		ImGui::ColorEdit4On32Bit("End color", (unsigned char*)&color2, alpha_flag);

		if (ImGui::Button("Swap colors"))
		{
			int temp = color2;
			color2 = color1;
			color1 = temp;
		}

		if (ImGui::Button("Generate gradient"))
		{
			DisableHighlightModes();
			GenerateGradient(idx1, idx2, color1, color2);
		}

		ImGui::EndPopup();
	}
}

void PaletteEditorWindow::GenerateGradient(int idx1, int idx2, int color1, int color2)
{
	idx1 -= 1;
	idx2 -= 1;

	int steps = idx2 - idx1;
	if (steps < 1)
	{
		return;
	}

	float frac = 1.0 / (float)(idx2 - idx1);

	unsigned char a1 = (color1 & 0xFF000000) >> 24;
	unsigned char a2 = (color2 & 0xFF000000) >> 24;
	unsigned char r1 = (color1 & 0xFF0000) >> 16;
	unsigned char r2 = (color2 & 0xFF0000) >> 16;
	unsigned char g1 = (color1 & 0xFF00) >> 8;
	unsigned char g2 = (color2 & 0xFF00) >> 8;
	unsigned char b1 = color1 & 0xFF;
	unsigned char b2 = color2 & 0xFF;

	((int*)m_paletteEditorArray)[idx1] = color1;

	for (int i = 1; i <= steps; i++)
	{
		int a = ((int)((a2 - a1) * i * frac + a1) & 0xFF) << 24;
		int r = ((int)((r2 - r1) * i * frac + r1) & 0xFF) << 16;
		int g = ((int)((g2 - g1) * i * frac + g1) & 0xFF) << 8;
		int b = (int)((b2 - b1) * i * frac + b1) & 0xFF;
		int color = r | g | b;

		((int*)m_paletteEditorArray)[idx1 + i] = color ^ ((int*)m_paletteEditorArray)[idx1 + i] & a;
	}

	g_interfaces.pPaletteManager->ReplacePaletteFile(m_paletteEditorArray, m_selectedFile, *m_selectedCharPalHandle);
}