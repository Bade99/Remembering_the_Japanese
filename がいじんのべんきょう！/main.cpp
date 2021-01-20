//-------------------General TODOs-------------------:
//TODO(fran): it may be a good idea to give the user the option to allow the application to open itself to make you perform a quick practice, maybe not open itself but show a toast msg
//TODO(fran): it may be a good idea to simply stop #including windows.h and instead just include specifically the files I want
//TODO(fran): languages shouldnt be in the resource file, we should store them in separate files on the user's disc, check on startup for the langs available and load/unload the one the user wants

//-----------------Macro Definitions-----------------:
#ifdef _DEBUG
//TODO(fran): change to logging
#define _SHOWCONSOLE /*Creates a cmd window for the current process, allows for easy output through printf or similar*/
#endif


//---------------------Includes----------------------:
#include "windows_sdk.h"
#include <Shlwapi.h> //StrCpyNW

#include "がいじんの_Helpers.h"
#include "unCap_Renderer.h"
#include "unCap_Global.h"
#include "LANGUAGE_MANAGER.h"

//----------------------Linker-----------------------:
#pragma comment(lib,"shlwapi.lib") //strcpynw (shlwapi.h)

//----------------------Globals----------------------:
i32 n_tabs = 0;//Needed for serialization

UNCAP_COLORS unCap_colors{ 0 };
UNCAP_FONTS unCap_fonts{ 0 }; //TODO(fran): font info should be saved and end user editable

//-------------Application Entry Point---------------:
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    urender::init(); defer{ urender::uninit(); }; //TODO(fran): this shouldnt be needed


#ifdef _SHOWCONSOLE
    AllocConsole();
    FILE* ___s; defer{ fclose(___s); };
    freopen_s(&___s, "CONIN$", "r", stdin);
    freopen_s(&___s, "CONOUT$", "w", stdout);
    freopen_s(&___s, "CONOUT$", "w", stderr);
#endif


    LOGFONTW lf{ 0 };
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfHeight = -15;//TODO(fran): parametric
    //INFO: by default if faceName is not set then "Modern" is used, looks good but lacks some charsets
    StrCpyNW(lf.lfFaceName, GetApp_FontFaceName().c_str(), ARRAYSIZE(lf.lfFaceName));

    unCap_fonts.General = CreateFontIndirect(&lf); runtime_assert(unCap_fonts.General, (str(L"Font not found")+ lf.lfFaceName).c_str()); //TODO(fran): add a runtime_msg that does exactly the same as runtime_assert except for crashing

    lf.lfHeight = (LONG)((float)GetSystemMetrics(SM_CYMENU) * .85f); //TODO(fran): why isnt this negative?

    unCap_fonts.Menu = CreateFontIndirectW(&lf); runtime_assert(unCap_fonts.Menu, (str(L"Font not found") + lf.lfFaceName).c_str());



    LANGUAGE_MANAGER& lang_mgr = LANGUAGE_MANAGER::Instance(); lang_mgr.SetHInstance(hInstance);
    constexpr cstr serialization_folder[] = L"\\がいじんのべんきょう！";
    {
        const str to_deserialize = load_file_serialized(serialization_folder);
        _BeginDeserialize();
        _deserialize_struct(lang_mgr, to_deserialize);
        _deserialize_struct(unCap_colors, to_deserialize);
        default_colors_if_not_set(&unCap_colors);
        defer{ for (HBRUSH& b : unCap_colors.brushes) if (b) { DeleteObject(b); b = NULL; } };
    }

    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        Assert(bRet != -1);//there was an error

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    {
        str serialized;
        _BeginSerialize();
        serialized += _serialize_struct(lang_mgr);
        serialized += _serialize_struct(unCap_colors);

        save_to_file_serialized(serialized, serialization_folder);
    }
    return (int)msg.wParam;
}

//What my database needs:
//- "Selects" will be the heart of the program, there will be 2 main searches:
//      -by the text string (mainly in japanese but could maybe be in the translated lang as well) 
//      -by date (we want to pick the least recently shown words for the user to recall, and also the newest, I remember hearing that for words to stick the method is showing it at growing intervals: after 1 day then 3, 7, ... though I should read more on that)
//      -Also we want to be able to hear from the user, say they search a word and ask us to remember it for them, in that case we have two options, add them to a list of priority or update some of it's date parameters to make it show up, not sure I love the second option since we'd have to forge values, also the priority list looks better for storing words that the user didnt get right when practicing
//- "Updates" will be common since the date values will be changing often
//- Third in commonality will be "inserts" of new words
//- Lastly "deletes", most users will probably never delete anything unless they made a mistake

//Table structure: id | word_jp | word_translated | word_kanji |　word_type (noun,verb,...) | date_added | date_last_shown ; also the same japanese word can have multiple meanings, so word_translated and word_type should be a list

//Apart from that we want a "groups" table, the user may want to group different words by some category, eg questions: 何,どこ,...

//interesting comment on hashtables and crc: https://stackoverflow.com/questions/17107324/database-creation-using-c-programming

//I'd guess I'd prefer the db to exist entirely on ram (since even a huge db wont be much more than 100MB) and have a secondary thread that takes care of sending it back to disk when some condition is met. That would make it much easier to manage, no partitioning needed

//Also Im looking at SQLite which looks to use simply a file that you open and that's it