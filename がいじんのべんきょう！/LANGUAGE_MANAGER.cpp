#include "LANGUAGE_MANAGER.h"

bool LANGUAGE_MANAGER::deserialize(str name, const str& content) {
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


BOOL LANGUAGE_MANAGER::AddDynamicText(HWND hwnd, UINT messageID)
{
	if (!hwnd) return FALSE;
	this->DynamicHwnds[hwnd] = messageID;
	this->UpdateDynamicHwnd(hwnd, messageID);
	return TRUE;
}

BOOL LANGUAGE_MANAGER::AddMenuDrawingHwnd(HWND MenuDrawer)
{
	//TODO(fran): check HWND is valid
	if (MenuDrawer) {
		this->MenuDrawers.push_back(MenuDrawer);
		return TRUE;
	}
	return FALSE;
}

BOOL LANGUAGE_MANAGER::AddMenuText(HMENU hmenu, UINT_PTR ID, u32 stringID)
{
	BOOL res = UpdateMenu(hmenu, ID, stringID);
	if (res) this->Menus[std::make_pair(hmenu, ID)] = stringID;
	return res;
}

//LANGUAGE LANGUAGE_MANAGER::GetCurrentLanguage()
//{
//	return this->CurrentLanguage;
//}

BOOL LANGUAGE_MANAGER::AddWindowText(HWND hwnd, u32 stringID)
{
	BOOL res = UpdateHwnd(hwnd, stringID);
	if (res) this->Hwnds[hwnd] = stringID;
	return res;
}

BOOL LANGUAGE_MANAGER::AddComboboxText(HWND hwnd, UINT ID, u32 stringID)
{
	BOOL res = UpdateCombo(hwnd, ID, stringID);
	if (res) this->Comboboxes[std::make_pair(hwnd, ID)] = stringID;
	return res;
}

bool LANGUAGE_MANAGER::ChangeLanguage(str newLang)
{
	bool res = this->IsValidLanguage(newLang);
	if (res) {
		this->CurrentLanguage = newLang;

		this->Mapper = load_file_lang((utf16*)this->LangFolder.c_str(), (utf16*)this->CurrentLanguage.c_str());

		for (auto const& hwnd_sid : this->Hwnds)
			this->UpdateHwnd(hwnd_sid.first, hwnd_sid.second);

		for (auto const& hwnd_id_sid : this->Comboboxes)
			this->UpdateCombo(hwnd_id_sid.first.first, hwnd_id_sid.first.second, hwnd_id_sid.second);

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

std::wstring LANGUAGE_MANAGER::RequestString(u32 stringID)
{
	str res;
	try {
		res = this->Mapper.map.at(stringID);
	}
	catch (...) { res = L"INVALID ID"; }
	return res;
}
