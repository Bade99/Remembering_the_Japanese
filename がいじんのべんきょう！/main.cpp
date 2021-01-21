//-------------------General TODOs-------------------:
//TODO(fran): it may be a good idea to give the user the option to allow the application to open itself to make you perform a quick practice, maybe not open itself but show a toast msg
//TODO(fran): languages shouldnt be in the resource file, we should store them in separate files on the user's disc, check on startup for the langs available and load/unload the one the user wants
//TODO(fran): get rid of languages from the resource file, we should create files for the langs we have, and then have the lang_mgr read the folder and retrieve all the files in it, that way we also get rid of the language enums, and extra langs can be easily added after the fact, we simply retrieve the name of the language either from the filename or the first line inside the file. On each language change we have lang_mgr load the needed file into memory and parse it, we can have a very simple format: each line contains a number that will be the key, then a tab or some other separator, and then the string, additionally to support things like line breaks we parse the string to find \n, \t, ... and convert it to the single character it wants to represent

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
#include "lang.h"
#include "LANGUAGE_MANAGER.h"

#include "sqlite3.h" //now this is what programming is about, a _single_ 8MB file added just about 2sec to my compile time and compiled with no errors the first time I tried, there's probably some config I can do but this is already beyond perfection, also im on windows and using msvc 2019, the worst possible case probably

#include "がいじんの_べんきょう.h"

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

    //TODO(fran): global var work_folder that is set up on startup and stores, in this case, the path to appdata\our folder
    lang def_langs[] = {
        { lang_english , lang_english_entries},
        { lang_español , lang_español_entries},
    };

    LANGUAGE_MANAGER& lang_mgr = LANGUAGE_MANAGER::Instance(); lang_mgr.SetHInstance(hInstance);

    cstr* work_folder; defer{ free(work_folder); };
    {
        constexpr cstr app_folder[] = L"\\がいじんのべんきょう！";
        PWSTR general_folder;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &general_folder);
        size_t general_folder_sz = cstr_len(general_folder) * sizeof(*general_folder);
        work_folder = (cstr*)malloc(general_folder_sz + sizeof(app_folder));
        memcpy(work_folder, general_folder, general_folder_sz);
        memcpy((void*)(((u8*)work_folder)[general_folder_sz]), app_folder, sizeof(app_folder));
        CoTaskMemFree(general_folder);
    }


    {
        const str to_deserialize = load_file_serialized(work_folder);
        _BeginDeserialize();
        _deserialize_struct(lang_mgr, to_deserialize);
        _deserialize_struct(unCap_colors, to_deserialize);
        default_colors_if_not_set(&unCap_colors);
        defer{ for (HBRUSH& b : unCap_colors.brushes) if (b) { DeleteObject(b); b = NULL; } };
    }

    //NOTE: at least for now we'll only have one user, when the time comes and we feel that's needed we'll simply add an intermediate folder for each user into which a separate db will be stored
    sqlite3* db;
    constexpr cstr db_name[] = L"\\db";
    str db_path = str(work_folder) + db_name; 

    //TODO(fran): im kind of dissatisfied with them not having a sqlite3_open_v2 for utf16 so I can do some configs
    int open_db_res = sqlite3_open16(db_path.c_str(), &db); defer{ sqlite3_close(db); };
    sqliteok_runtime_assert(open_db_res,db);

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

        save_to_file_serialized(serialized, work_folder);
    }
    return (int)msg.wParam;
}

//What my database needs:
//- "Selects" will be the heart of the program, there will be 2 main searches:
//      -by the text string (mainly in japanese but could maybe be in the translated lang as well) 
//      -by date (aka unix time) (we want to pick the least recently shown words for the user to recall, and also the newest, I remember hearing that for words to stick the method is to show them at growing intervals: 1 day after creation, then 3, 7, ... though I should read more on that)
//      -Also we want to be able to hear from the user, say they search a word and ask us to remember it for them, in that case we have two options, add them to a list of priority or update some of it's date parameters to make it show up, not sure I love the second option since we'd have to forge values, also the priority list looks better for storing words that the user didnt get right when practicing
//- "Updates" will be common since the date values will be changing often
//- Third in commonality will be "inserts" of new words
//- Lastly "deletes", most users will probably never delete anything unless they made a mistake

//Table structure: id | word_jp | word_translated | word_kanji |　word_type (noun,verb,...) | date_added | date_last_shown ; also the same japanese word can have multiple meanings, so word_translated and word_type should be a list

//Apart from that we want a "groups" table, the user may want to group different words by some category, eg questions: 何,どこ,...

//interesting comment on hashtables and crc: https://stackoverflow.com/questions/17107324/database-creation-using-c-programming

//I'd guess I'd prefer the db to exist entirely on ram (since even a huge db wont be much more than 100MB) and have a secondary thread that takes care of sending it back to disk when some condition is met. That would make it much easier to manage, no partitioning needed

//Bailing out and going with an already established db application, for the moment... : (it must be as "low friction" and simple/small as possible)

//-SQLite: very simple to add to a project and to use, it's 4 files and I think we only need 2, pretty close to 1

//-Redis with a hash table may be another option, but it has one too many dependencies and many many many files

//-SQL compact doesnt look very c programmer friendly, more c# oriented

//Any of this solutions are going to be potentially slower than one I make for at least one reason, no concurrency, I only got one user editing their db at one time, so no mutex-like behaviour is needed, also I dont know whether I can make them work with utf16 instead of utf8, which incurs a translation cost, and a couple other reasons

//NOTE: when the user requests us to make them remember that word we should do the same as if the word had just been created, show it after 1 day, after 3 days, etc. Also this feature should discriminate between each possible translation, say one jp word has a noun and an adjective registered, then both should work as separate entities, aka the user can ask for one specific translation to be put on the priority queue, we got a "one to many" situation


//NOTE: as an aside this application also servers as a test so see whether any part on the compilation/execution/version control pipeline fails due to the unusual japanese characters