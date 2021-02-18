//-------------------General TODOs-------------------:
//TODO(fran): it may be a good idea to give the user the option to allow the application to open itself to make you perform a quick practice, maybe not open itself but show a toast msg
//TODO(fran): batch file for compiling that way too
//TODO(fran): go straight to utf8 db since there's basically no utf16 support in sqlite. NOTE: im not to sure of this, there are ways to get utf16, so there's support for getting stuff in utf16 but no for sending it, unfortunate but ok at least it's half the work
//TODO(fran): should I stop giving suggestions through the IME window? so the user isnt dependent on the correct writing of the IME suggestions, at least for something like the searchbar
//TODO(fran): we probably also want a "forward" button
//TODO(fran): dpi awareness https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows https://github.com/tringi/win32-dpi
//TODO(fran): add a header for all ProcStates with a unique key for each proc that way it can later identify the state is the corresponding one to that wnd, eg:
//                                      struct ProcStateHeader {
//                                          HWND wnd;
//                                          HWND parent;
//                                          u32 id;//or some other name
//                                      };
 //TODO(fran): font info should be saved and be end user editable

//BIG TODO(fran): we can actually pre allocate the sizes for everything, and assign "static" structures to sections of the code for reuse, going back to the handmade hero mentality that'd help a lot, no need for mallocs all over the place, at most we should implement re allocation, but even that's not necessary if we fixed every size, I was thinking about differentiating current size and allocated size for things like strings, but that's also not needed, with fixed sizes we can have everything allocated on the correct size from the start, the content on the pages of this program is very much fixed and controllable
//BIG TODO(fran): make a resizer, that'd save lots of problems
//BIG TODO(fran): db configuration, pragmas and the like (eg the page size is important to be at least 4KB)
//BIG TODO(fran): add ability to create word groups, lists of known words the user can create and add to, also ask to practice a specific group

//-------------------Stuff I learnt-------------------:
//fran:
//If you have no .rc file and try to load something from it (eg LoadStringW) then the antivirus, in my case kaspersky, detects the exe as VHO:Exploit.Win32.Convagent.gen and sometimes it even says it's a Trojan
//IMPORTANT: Unsurprisingly enough the biggest problem I had by using japanese on my paths and files was with the Resource editor, it seems to always default to ascii and write paths in ascii also, which means you cant get to your files!!! If I ever get someone else working on this I must change the way I handle it right now, which is basically writing the .rc file myself, also I cant ever add something to that rc or what I wrote to fix it will be overriden. One idea I got was setting the lang for the resource, say I add an icon, then if I say that icon belongs to the japanese lang it might then use the correct codepage and get the correct filepath. As an extra im now very confused about the .rc files, people say you should version control them but they clearly have my filepath hardcoded in there, so other people will have theirs, that's a recipe for disaster in my book. I should probably try with the resx format, maybe you have more control over that one (Extra extra note, thanks to putting the assets folder outside new devs can get their .rc file to work without problem, the path will only contain ascii, look at the github page if you dont understand why). Raymond's solution works https://devblogs.microsoft.com/oldnewthing/20190607-00/?p=102569 if I dont update the file. I had to give up and change to name of the folder Im working on from がいじんのべんきょう！ to Remembering_the_Japanese, this is pathetic visual studio, I cant comprehend how they have encoding problems in 2020, this should've been fixed 10 years ago!!!!
//This isnt anything new but simply a reflective moment that came to me, how each time I stopped using a windows feature the arquitecting came so much easier, the systems put in place for handling stuff are terrible, either cause they are or simply because Im outside windows and dont follow their design method, by creating my own, much simpler, much less interdependent system I could do stuff much faster, more efficient, and easier
//Unix time has always been signed! Reasons for it: apparently c didnt support unsigned numbers at the time, also being signed allows you to go before 1970, not only forwards, which at the time might have seemed important since they were close to "before 1970", finally that means that you have 31 bits to go forward which leaves you at 2038, when going 64 bits you get 63 to go forward which leaves you beyond what anybody reading this will ever see
//I do hate about all this SQL like databases that you need every param converted to a string back and forth, but mainly it bothers me on insertion, if I want to put a number I gotta convert it to string, there's no printf like function where you could put all the values in their real format, therefore you never get efficiency benefits from columns that arent text, really all columns should be text if there isnt gonna be a good support from the start for the values to be inserted in their non text form, so that'd be a future personal challenge, make a better db
//The Property Sheet control is what you use when you want to create a wizard, ej install wizard/installer, also it seems to give the tab control functionality for real, unlike the wrongly named "tab control" which does very little
//Windows' hidden window classes (ComboLBox,...) https://docs.microsoft.com/en-us/windows/win32/winmsg/about-window-classes?redirectedfrom=MSDN#system
//More undocumented windows WM_UAH... https://github.com/adzm/win32-custom-menubar-aero-theme

//-----------------Macro Definitions-----------------:
#define _CRT_SECURE_NO_WARNINGS /*Dont take away functions for which you dont have replacement, and Im not stupid*/
#ifdef _DEBUG
//TODO(fran): change to logging
#define _SHOWCONSOLE /*Creates a cmd window for the current process, allows for easy output through printf or similar*/
//#define _DELETE_DB
#endif

//---------------------Includes----------------------:
#include "windows_sdk.h"
#include <Shlwapi.h> //StrCpyNW

#include "win32_Helpers.h"
#include "win32_Global.h"
#include "lang.h"
#include "LANGUAGE_MANAGER.h"
#include "img.h"

#include "sqlite3.h" //now this is what programming is about, a _single_ 8MB file added just about 2sec to my compile time and compiled with no errors the first time I tried, there's probably some config I can do but this is already beyond perfection, also im on windows and using msvc 2019, the worst possible case probably

#include "win32_nonclient.h"
#include "がいじんの_べんきょう.h"

//----------------------Linker-----------------------:
#pragma comment(lib,"shlwapi.lib") //strcpynw (shlwapi.h)
#pragma comment(lib,"comctl32.lib") //loadiconmetric (commctrl.h)
#pragma comment(lib,"Imm32.lib") // IME related stuff

//INFO: this is actually necessary, if you simply link to comctl32.lib you'll get runtime errors of "ordinals not found", in my case ordinal 380, TODO(fran): what we could do is lower the version to allow from more systems to run the sw, other thing would be making my own loadiconmetric, that's the only function that complained that I had to link
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") //for multiline edit control

//-------------Application Entry Point---------------:
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

    //-------------Debug Output Console-------------:
#ifdef _SHOWCONSOLE
    AllocConsole();
    FILE* ___s; defer{ fclose(___s); };
    freopen_s(&___s, "CONIN$", "r", stdin);
    freopen_s(&___s, "CONOUT$", "w", stdout);
    freopen_s(&___s, "CONOUT$", "w", stderr);
#endif

    //-----------------Font Setup------------------:
    LOGFONTW lf{ 0 };
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfHeight = -15;//TODO(fran): parametric
    //INFO: by default if faceName is not set then "Modern" is used, looks good but lacks some charsets
    StrCpyNW(lf.lfFaceName, GetApp_FontFaceName().c_str(), ARRAYSIZE(lf.lfFaceName));

    global::fonts.General = CreateFontIndirect(&lf); runtime_assert(global::fonts.General, (str(L"Font not found: ")+ lf.lfFaceName).c_str()); //TODO(fran): add a runtime_msg that does exactly the same as runtime_assert except for crashing

    lf.lfHeight = (LONG)((float)GetSystemMetrics(SM_CYMENU) * .85f); //TODO(fran): why isnt this negative?

    global::fonts.Menu = CreateFontIndirectW(&lf); runtime_assert(global::fonts.Menu, (str(L"Font not found: ") + lf.lfFaceName).c_str());

    {
        HDC dc = GetDC(0); defer{ ReleaseDC(0,dc); };
        HFONT old_font = SelectFont(dc, GetStockFont(DEFAULT_GUI_FONT)); defer{ SelectFont(dc,old_font); };//TODO(fran): pretty shitty solution
        TEXTMETRIC tm; GetTextMetrics(dc, &tm);
        lf.lfHeight = tm.tmHeight;
    }
    global::fonts.Caption = CreateFontIndirectW(&lf); runtime_assert(global::fonts.Menu, (str(L"Font not found: ") + lf.lfFaceName).c_str());

    defer{ for (auto& f : global::fonts.all) if (f) { DeleteObject(f); f = NULL; } };

    //----------------Bitmap Setup-----------------:
    //INFO: use CreateBitmap for monochrome ones, and CreateCompatibleBitmap for color
#define create_global_bmps(bmp) global::bmps.bmp = CreateBitmap(bmp.w, bmp.h,1,bmp.bpp,bmp.mem); runtime_assert(global::bmps.bmp,(str(L"Failed to create bitmap: ") + L#bmp).c_str()) /*TODO(fran): very ugly, but I need to assert on the result of CreateBmp*/


    create_global_bmps(close);//TODO(fran): why isnt this autogenerated with foreach?
    create_global_bmps(maximize);
    create_global_bmps(minimize);
    create_global_bmps(tick);
    create_global_bmps(arrow_right);
    create_global_bmps(dropdown);
    create_global_bmps(circle);
    create_global_bmps(bin);
    create_global_bmps(arrowLine_left);
    create_global_bmps(arrowSimple_right);

    defer{ for (auto& bmp : global::bmps.all) if(bmp){ DeleteObject(bmp); bmp = NULL; } };

    //----------------Work Folder Setup-----------------:
    cstr* work_folder; defer{ free(work_folder); };
    {
        constexpr cstr app_folder[] = L"\\がいじんのべんきょう！";
        PWSTR general_folder;
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &general_folder);
        size_t general_folder_sz = cstr_len(general_folder) * sizeof(*general_folder);
        work_folder = (cstr*)malloc(general_folder_sz + sizeof(app_folder));
        memcpy(work_folder, general_folder, general_folder_sz);
        memcpy((void*)(((u8*)work_folder)+general_folder_sz), app_folder, sizeof(app_folder));
        CoTaskMemFree(general_folder);
    }
    CreateDirectoryW(work_folder, 0);

    //-----------------Language Setup------------------:
    LANGUAGE_MANAGER& lang_mgr = LANGUAGE_MANAGER::Instance();
    {
        str lang_folder = str(work_folder) + L"\\lang";

        def_lang def_langs[] = {
            { lang_english , lang_english_entries, sizeof(lang_english_entries)},
            { lang_español , lang_español_entries, sizeof(lang_español_entries)},
        };
        save_to_file_langs(def_langs, ARRAYSIZE(def_langs), (utf16*)lang_folder.c_str());//TODO(fran): having to write to disk every time is kind of annoying and slow

        lang_mgr.SetLanguageFolder(std::move(lang_folder));
    }

    //-----------------Deserialization------------------:
    べんきょうSettings べんきょう_cl;
    auto& win32_colors = global::colors;//NOTE: I avoid direclty using global::colors as the serialization name cause of the :: which could later be used as an asignment operator
    {
        const str to_deserialize = load_file_serialized(work_folder);
        _BeginDeserialize();
        _deserialize_struct(lang_mgr, to_deserialize);
        _deserialize_struct(べんきょう_cl, to_deserialize);
        //_deserialize_struct(unCap_colors, to_deserialize);
        //default_colors_if_not_set(&unCap_colors);

        _deserialize_struct(win32_colors, to_deserialize);
        default_colors_if_not_set(&win32_colors);

    }
    //defer{ for (auto& br : unCap_colors.all) if (br) { DeleteObject(br); br = NULL; } };
    defer{ for (auto& br : win32_colors.all) if (br) { DeleteObject(br); br = NULL; } };


    //------------------Database Setup-------------------:
    //NOTE: at least for now we'll only have one user, when the time comes and we feel that's needed we'll simply add an intermediate folder for each user into which a separate db will be stored
    sqlite3* db;
    constexpr cstr db_name[] = 
#ifdef _DEBUG /*TODO(fran): this aint too bulletproof*/
        L"\\test-db.db";//NOTE: I simply put an extension so external programs pick up the file on windows' file select
#else 
        L"\\db";
#endif
    str db_path = str(work_folder) + db_name; 

#if defined(_DEBUG) && defined(_DELETE_DB) //Delete db file if exists
    DeleteFileW(db_path.c_str());
#endif

    //TODO(fran): im kind of dissatisfied with them not having a sqlite3_open_v2 for utf16 so I can do some configs
    int open_db_res = sqlite3_open16(db_path.c_str(), &db);
    sqliteok_runtime_assert(open_db_res,db);
    defer{ 
        int close_res = sqlite3_close(db); sqliteok_runtime_check(close_res,db); 
#if defined(_DEBUG) && defined(_DELETE_DB)
        DeleteFileW(db_path.c_str());
#endif
    };

    //----------------Window Setup-----------------:
    RECT べんきょう_nc_rc = UNCAPNC_calc_nonclient_rc_from_client(べんきょう_cl.rc, FALSE);
    べんきょう_cl.db = db;

    unCapNcLpParam べんきょう_nclpparam;
    べんきょう_nclpparam.client_class_name = べんきょう::wndclass;
    べんきょう_nclpparam.client_lp_param = &べんきょう_cl;

    HWND べんきょう_nc = CreateWindowEx(WS_EX_CONTROLPARENT, nc::wndclass, global::app_name, WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        べんきょう_nc_rc.left, べんきょう_nc_rc.top, RECTWIDTH(べんきょう_nc_rc), RECTHEIGHT(べんきょう_nc_rc), nullptr, nullptr, hInstance, &べんきょう_nclpparam);
    Assert(べんきょう_nc);

    べんきょう::set_brushes(UNCAPNC_get_state(べんきょう_nc)->client, TRUE, global::colors.ControlBk); //TODO(fran): having to do this here aint too nice, but at least is nicer than having to do init_wndclass for everything

    UpdateWindow(べんきょう_nc);

    //------------------Main Loop-------------------:
    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        Assert(bRet != -1);//there was an error

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //----------------Serialization-----------------:
    {
        str serialized;
        _BeginSerialize();
        serialized += _serialize_struct(lang_mgr);
        //serialized += _serialize_struct(unCap_colors);
        serialized += _serialize_struct(べんきょう_cl);
        serialized += _serialize_struct(win32_colors);

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

//NOTE: as an aside this application also serves as a test so see whether any part on the compilation/execution/version control pipeline fails due to the unusual japanese characters