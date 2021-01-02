#include "plugin.h"
#include "parser.h"

enum
{
    MENU_DISASM_PASTE,
	MENU_DISASM_PASTE_PATCH,
    MENU_DUMP_PASTE,
	MENU_DUMP_PASTE_PATCH,
	MENU_DISASM_ABOUT,
	MENU_DUMP_ABOUT
};


#define ABOUT_MSG "yummyPaste by Oguz Kartal\r\n\r\n" \
				  "paste your shellcode string into the x64dbg!\r\n\r\nhttps://oguzkartal.net\r\n" \
				  "compiled in: "  __DATE__ " " __TIME__


void About()
{
	MessageBoxA(hwndDlg, ABOUT_MSG, "yummyPaste", MB_ICONINFORMATION);
}

LPSTR GetClipboardTextData(size_t *pLength)
{
	LPSTR temp=NULL,pastedContent = NULL;
	size_t contentLength = 0;
	HANDLE cbHandle = NULL;

	if (pLength)
		*pLength = 0;

	if (!OpenClipboard(hwndDlg))
	{
		MessageBoxA(hwndDlg, "The clipboard couldn't be opened","yummyPaste",MB_ICONWARNING);
		goto oneWayExit;
	}


	if (!IsClipboardFormatAvailable(CF_TEXT))
	{
		goto oneWayExit;
	}

	cbHandle = GetClipboardData(CF_TEXT);

	if (!cbHandle)
	{
		MessageBoxA(hwndDlg, "Clipboard data couldn't readed", "yummyPaste", MB_ICONWARNING);
		goto oneWayExit;
	}



	temp = (LPSTR)GlobalLock(cbHandle);

	if (!temp)
	{
		MessageBoxA(hwndDlg, "The data couldn't be extracted from the cb object", "yummyPaste", MB_ICONSTOP);
		goto oneWayExit;
	}

	contentLength = strlen(temp);

	if (contentLength == 0)
		goto oneWayExit;

	pastedContent = (LPSTR)Malloc(contentLength + 1);


	if (!pastedContent)
	{
		goto oneWayExit;
	}

	strcpy_s(pastedContent, contentLength+1, temp);

	
	if (pLength)
		*pLength = contentLength;

oneWayExit:

	if (temp)
	{
		GlobalUnlock(cbHandle);
		temp = NULL;
	}

	CloseClipboard();

	return pastedContent;
}



void MakeTomatoPaste(int window,BOOL patched)
{
	size_t pdLen = 0;
	LPSTR pasteData = NULL;
	SELECTIONDATA sel = { 0 };
	BINARY_DATA *binary = NULL;

	if (!DbgIsDebugging())
	{
		MessageBoxA(hwndDlg, "Where is your tomato to be paste?", "yummyPaste", MB_ICONWARNING);
		return;
	}
	
	GuiSelectionGet(window, &sel);

	
	pasteData = GetClipboardTextData(&pdLen);

	if (!pasteData)
		return;

	ResetBinaryObject();

	ParseBytes(pasteData, pdLen);

	binary = GetBinaryData();

	if (binary->invalid)
	{
		MessageBoxA(hwndDlg, "looks like your content is invalid to me", "yummyPaste", MB_ICONWARNING);
		Free(pasteData);
		return;
	}

	if(patched)
		DbgFunctions()->MemPatch(sel.start, binary->binary, binary->index);
	else
		DbgMemWrite(sel.start, binary->binary, binary->index);
	
	Free(pasteData);

	if (window == GUI_DISASSEMBLY)
		GuiUpdateDisassemblyView();
	else if (window == GUI_DUMP)
		GuiUpdateDumpView();

}

PLUG_EXPORT void CBINITDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info)
{
}

PLUG_EXPORT void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info)
{
}

PLUG_EXPORT void CBEXCEPTION(CBTYPE cbType, PLUG_CB_EXCEPTION* info)
{
}

PLUG_EXPORT void CBDEBUGEVENT(CBTYPE cbType, PLUG_CB_DEBUGEVENT* info)
{
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info)
{
    switch(info->hEntry)
    {

    case MENU_DISASM_PASTE:
        MakeTomatoPaste(GUI_DISASSEMBLY, FALSE);
        break;
	case MENU_DISASM_PASTE_PATCH:
		MakeTomatoPaste(GUI_DISASSEMBLY, TRUE);
		break;
    case MENU_DUMP_PASTE:
        MakeTomatoPaste(GUI_DUMP,FALSE);
        break;
	case MENU_DUMP_PASTE_PATCH:
		MakeTomatoPaste(GUI_DUMP, TRUE);
		break;
	case MENU_DISASM_ABOUT:
	case MENU_DUMP_ABOUT:
		About();
		break;
    default:
        break;
    }
}

//Initialize your plugin data here.
bool pluginInit(PLUG_INITSTRUCT* initStruct)
{
	if (!InitBinaryObject(0xFEED))
	{
		MessageBoxA(hwndDlg, "Ups. memory?", "yummyPaste", MB_ICONSTOP);
		return false;
	}


    return true; //Return false to cancel loading the plugin.
}


void pluginStop()
{
	DestroyBinaryObject();
}

void pluginSetup()
{
    _plugin_menuaddentry(hMenuDisasm, MENU_DISASM_PASTE, "&Paste it!");
	_plugin_menuaddentry(hMenuDisasm, MENU_DISASM_PASTE_PATCH, "Paste and Patch");
    _plugin_menuaddentry(hMenuDump, MENU_DUMP_PASTE, "&Paste it!");
	_plugin_menuaddentry(hMenuDump, MENU_DUMP_PASTE_PATCH, "Paste and Patch");
	_plugin_menuaddentry(hMenuDisasm, MENU_DISASM_ABOUT, "A&bout");
	_plugin_menuaddentry(hMenuDump, MENU_DUMP_ABOUT, "A&bout");

}
