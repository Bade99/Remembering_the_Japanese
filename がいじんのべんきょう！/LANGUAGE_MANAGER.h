#pragma once
#include "windows_sdk.h"
#include "がいじんの_Platform.h"
#include <map>
#include <vector>
#include "がいじんの_Helpers.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "lang.h"

//Request string
#define RS(stringID) LANGUAGE_MANAGER::Instance().RequestString(stringID)

//Request c-string
#define RCS(stringID) LANGUAGE_MANAGER::Instance().RequestString(stringID).c_str()

//Add window string
#define AWT(hwnd,stringID) LANGUAGE_MANAGER::Instance().AddWindowText(hwnd, stringID)

//Add combobox string in specific ID (Nth element of the list)
#define ACT(hwnd,ID,stringID) LANGUAGE_MANAGER::Instance().AddComboboxText(hwnd, ID, stringID);

//Add combobox cuebanner string (default text when there's no selection)
#define ACC(hwnd,stringID) LANGUAGE_MANAGER::Instance().AddComboboxCuebanner(hwnd, stringID);

//Add menu string
#define AMT(hmenu,ID,stringID) LANGUAGE_MANAGER::Instance().AddMenuText(hmenu,ID,stringID);

class LANGUAGE_MANAGER
{
private:

#define _foreach_LANGUAGE_MANAGER_member(op) \
		op(str,CurrentLanguage,lang_english) \

	_foreach_LANGUAGE_MANAGER_member(_generate_member)

	str LangFolder; //folder where to look for language files
	std::vector<str> Langs; //List of valid languages
	lang_mapper Mapper; //mapper from id to string

public:

	static LANGUAGE_MANAGER& Instance()
	{
		static LANGUAGE_MANAGER instance;

		return instance;
	}
	LANGUAGE_MANAGER(LANGUAGE_MANAGER const&) = delete;
	void operator=(LANGUAGE_MANAGER const&) = delete;

	//INFO: all Add... functions send the text update message when called, for dynamic hwnds you should create everything first and only then add it

	//·Adds the hwnd to the list of managed hwnds and sets its text corresponding to stringID and the current language
	//·Many hwnds can use the same stringID
	//·Next time there is a language change the window will be automatically updated
	//·Returns FALSE if invalid hwnd or stringID //TODO(fran): do the stringID check
	BOOL AddWindowText(HWND hwnd, u32 stringID)
	{
		BOOL res = UpdateHwnd(hwnd, stringID);
		if (res) this->Hwnds[hwnd] = stringID;
		return res;
	}

	//·Updates all managed objects to the new language, all the ones added after this call will also use the new language
	bool ChangeLanguage(str newLang)
	{
		bool res = this->IsValidLanguage(newLang);
		if (res) {
			this->CurrentLanguage = newLang;

			this->Mapper = load_file_lang((utf16*)this->LangFolder.c_str(), (utf16*)this->CurrentLanguage.c_str());

			for (auto const& hwnd_sid : this->Hwnds)
				this->UpdateHwnd(hwnd_sid.first, hwnd_sid.second);

			for (auto const& hwnd_id_sid : this->Comboboxes)
				this->UpdateCombo(hwnd_id_sid.first.first, hwnd_id_sid.first.second, hwnd_id_sid.second);

			for (auto const& hwnd_sid : this->ComboboxesCuebanner)
				this->UpdateComboCuebanner(hwnd_sid.first, hwnd_sid.second);

			for (auto const& hwnd_msg : this->DynamicHwnds) {
				this->UpdateDynamicHwnd(hwnd_msg.first, hwnd_msg.second);
			}

			for (auto const& hmenu_id_sid : this->Menus) {
				this->UpdateMenu(hmenu_id_sid.first.first, hmenu_id_sid.first.second, hmenu_id_sid.second);
			}
			for (auto const& menudrawer : this->MenuDrawers) {
				DrawMenuBar(menudrawer);
				UpdateWindow(menudrawer);
			}
		}

		return res;
	}

	//·Returns the requested string in the current language
	//·If stringID is invalid returns L"" //TODO(fran): check this is true
	//INFO: uses temporary string that lives till the end of the full expression it appears in
	str RequestString(u32 stringID)
	{
		str res;
		try {
			res = this->Mapper.map.at(stringID);
		}
		catch (...) { res = str(L"INVALID ID ") + to_str(stringID); }
		return res;
	}

	//·Adds the hwnd to the list of managed comboboxes and sets its text for the specified ID(element in the list) corresponding to stringID and the current language
	//·Next time there is a language change the window will be automatically updated
	BOOL AddComboboxText(HWND hwnd, UINT ID, u32 stringID)
	{
		BOOL res = UpdateCombo(hwnd, ID, stringID);
		if (res) this->Comboboxes[std::make_pair(hwnd, ID)] = stringID;
		return res;
	}

	//·Adds the hwnd to the list of managed comboboxes and sets its default text corresponding to stringID and the current language
	//·Next time there is a language change the window will be automatically updated
	BOOL AddComboboxCuebanner(HWND hwnd, u32 stringID)
	{
		BOOL res = UpdateComboCuebanner(hwnd, stringID);
		if (res) this->ComboboxesCuebanner[hwnd] = stringID;
		return res;
	}

	//Add a control that manages other windows' text where the string changes
	//Each time there is a language change we will send a message with the specified code so the window can update its control's text
	//One hwnd can only be linked to one messageID
	BOOL AddDynamicText(HWND hwnd, UINT messageID)
	{
		if (!hwnd) return FALSE;
		this->DynamicHwnds[hwnd] = messageID;
		this->UpdateDynamicHwnd(hwnd, messageID);
		return TRUE;
	}

	//Menus are not draw by themselves, instead their owner window draws them, so if you want a menu to be redrawn you need to use this function
	//to indicate which window to call for its menus to be redrawn
	BOOL AddMenuDrawingHwnd(HWND MenuDrawer)
	{
		//TODO(fran): check HWND is valid
		if (MenuDrawer) {
			this->MenuDrawers.push_back(MenuDrawer);
			return TRUE;
		}
		return FALSE;
	}

	BOOL AddMenuText(HMENU hmenu, UINT_PTR ID, u32 stringID)
	{
		BOOL res = UpdateMenu(hmenu, ID, stringID);
		if (res) this->Menus[std::make_pair(hmenu, ID)] = stringID;
		return res;
	}

	str GetCurrentLanguage() { return CurrentLanguage; };

	void SetLanguageFolder(str folder) {
		LangFolder = folder;
		Langs = load_file_lang_names((utf16*)LangFolder.c_str());
	}

	_generate_default_struct_serialize(_foreach_LANGUAGE_MANAGER_member);

	bool deserialize(str name, const str& content) {
		bool res = false;
		str start = name + _keyvaluesepartor + _structbegin + _newline;
		size_t s = find_identifier(content, 0, start);
		size_t e = find_closing_str(content, s + start.size(), _structbegin, _structend);
		if (str_found(s) && str_found(e)) {
			s += start.size();
			str substr = content.substr(s, e - s);
			_foreach_LANGUAGE_MANAGER_member(_deserialize_member);
			res = true;
		}
		this->ChangeLanguage(this->CurrentLanguage);//whatever happens we need to set some language
		return res;
	}

private:
	LANGUAGE_MANAGER() {}
	~LANGUAGE_MANAGER() {}

	std::map<HWND, UINT> Hwnds;
	std::map<HWND, UINT> DynamicHwnds;
	std::map<std::pair<HWND, UINT>, UINT> Comboboxes;
	std::map<HWND, UINT> ComboboxesCuebanner;

	std::vector<HWND> MenuDrawers;
	std::map<std::pair<HMENU, UINT_PTR>, UINT> Menus;
	//Add list of hwnd that have dynamic text, therefore need to know when there was a lang change to update their text

	BOOL UpdateHwnd(HWND hwnd, u32 stringID)
	{
		BOOL res = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)this->RequestString(stringID).c_str()) == TRUE;
		InvalidateRect(hwnd, NULL, TRUE);
		return res;
	}

	BOOL UpdateDynamicHwnd(HWND hwnd, UINT messageID)
	{
		return PostMessage(hwnd, messageID, 0, 0);
	}

	BOOL UpdateCombo(HWND hwnd, UINT ID, u32 stringID)
	{
		UINT currentSelection = (UINT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);//TODO(fran): is there truly no better solution than having to do all this just to change a string?
		SendMessage(hwnd, CB_DELETESTRING, ID, 0);
		LRESULT res = SendMessage(hwnd, CB_INSERTSTRING, ID, (LPARAM)this->RequestString(stringID).c_str());
		if (currentSelection != CB_ERR) {
			SendMessage(hwnd, CB_SETCURSEL, currentSelection, 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return res != CB_ERR && res != CB_ERRSPACE;//TODO(fran): can I check for >=0 with lresult?
	}

	BOOL UpdateComboCuebanner(HWND hwnd, u32 stringID)
	{
		LRESULT _res = SendMessage(hwnd, CB_SETCUEBANNER, 0, (LPARAM)this->RequestString(stringID).c_str());
		bool res = _res==1;
		if (res) {
			InvalidateRect(hwnd, NULL, TRUE);//Just in case
		}
		return res;
	}

	BOOL UpdateMenu(HMENU hmenu, UINT_PTR ID, u32 stringID)
	{
		MENUITEMINFOW menu_setter;
		menu_setter.cbSize = sizeof(menu_setter);
		menu_setter.fMask = MIIM_STRING;
		std::wstring temp_text = this->RequestString(stringID);
		//menu_setter.dwTypeData = _wcsdup(this->RequestString(stringID).c_str()); //TODO(fran): can we avoid dupping, if not free memory
		menu_setter.dwTypeData = (LPWSTR)temp_text.c_str();
		BOOL res = SetMenuItemInfoW(hmenu, (UINT)ID, FALSE, &menu_setter);
		return res;
	}

	bool IsValidLanguage(str lang) {
		bool res = std::find(this->Langs.begin(), this->Langs.end(), lang) != this->Langs.end();
		return res;
	}

};

namespace userial {
	static str serialize(LANGUAGE_MANAGER& var) {//NOTE: we add this manually because we need a reference since the lang mgr is a singleton and doesnt allow for instancing
		return var.serialize();
	}
	static bool deserialize(LANGUAGE_MANAGER& var, str name, const str& content) {
		return var.deserialize(name, content);
	}
}
