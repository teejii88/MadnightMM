#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <ShlObj.h>

#include "mm_controls.h"

HWND mm_mod_location_label = { 0 };
HWND mm_mod_location_browse_button = { 0 };
HWND mm_mod_file_list = { 0 };

void mm_create_controls(HWND mmWindow, HINSTANCE hInstance)
{
	mm_mod_location_browse_button = CreateWindowEx(0, WC_BUTTON, _TEXT("Browse..."), BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, 10, 10, 80, 25, mmWindow, (HMENU)MM_CONTROL_MOD_LOCATION_BROWSE, hInstance, 0);
	SendMessage(mm_mod_location_browse_button, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), 0);

	mm_mod_location_label = CreateWindowEx(0, WC_EDIT, _TEXT(""), WS_CHILD | WS_VISIBLE | SS_LEFT | ES_READONLY, 100, 15, 650, 20, mmWindow, (HMENU)MM_CONTROL_MOD_LOCATION_LABEL, hInstance, 0);
	SendMessage(mm_mod_location_label, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), 0);

	mm_mod_file_list = CreateWindowEx(0, WC_LISTVIEW, _TEXT(""), WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT, 10, 40, 760, 400, mmWindow, (HMENU)MM_CONTROL_MOD_FILE_LIST, hInstance, 0);
	SendMessage(mm_mod_file_list, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), 0);

	// add a column
	LVCOLUMN lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvColumn.iSubItem = 0;
	lvColumn.pszText = "Enabled";
	lvColumn.cx = 75;
	lvColumn.fmt = LVCFMT_CENTER;

	SendMessage(mm_mod_file_list, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	lvColumn = { 0 };
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvColumn.iSubItem = 1;	
	lvColumn.pszText = "Mod Name";
	lvColumn.cx = 500;
	lvColumn.fmt = LVCFMT_LEFT;

	SendMessage(mm_mod_file_list, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);
}

void mm_control_handler(HWND mmWindow, WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
		case MM_CONTROL_MOD_LOCATION_BROWSE:
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));

			// get the path that we're gonna start from using the label
			TCHAR filePath[MAX_PATH] = _TEXT("");
			int len = GetWindowTextLength(GetDlgItem(mmWindow, MM_CONTROL_MOD_LOCATION_LABEL));
			if (len > 0)
				GetDlgItemText(mmWindow, MM_CONTROL_MOD_LOCATION_LABEL, filePath, MAX_PATH);

			// the info for the folder browser
			BROWSEINFO browseInfo = { 0 };
			browseInfo.lpszTitle = _T("Where are your mods stored?");
			browseInfo.hwndOwner = mmWindow;
			browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_EDITBOX;
			browseInfo.lpfn = mm_control_mod_browser_handler;
			browseInfo.lParam = (LPARAM)filePath;

			// fire the folder browser and then update our label if we get somewhere
			LPITEMIDLIST lpItemIDList = SHBrowseForFolder(&browseInfo);
			if (lpItemIDList)
			{
				// get the folder we chose and set the label
				SHGetPathFromIDList(lpItemIDList, filePath);
				SetDlgItemText(mmWindow, MM_CONTROL_MOD_LOCATION_LABEL, filePath);

				// free memory
				CoTaskMemFree(lpItemIDList);

				// go through the directory and find any .zip files we have
				mm_handle_mod_directory_found(mmWindow, filePath);
			}
		}
		break;
	}
}

static int CALLBACK mm_control_mod_browser_handler(HWND hWnd, UINT message, LPARAM lParam, LPARAM lpData)
{
	switch (message)
	{
		case BFFM_INITIALIZED:
		{
			// update our selection if we passed data
			if (lpData)
				SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
		}
		break;

		case BFFM_SELCHANGED:
		{
			TCHAR filePath[MAX_PATH] = { 0 };
			SHGetPathFromIDList(reinterpret_cast<LPITEMIDLIST>(lParam), filePath);

			// enable/disable ok button if we haven't got invalid file attributes
			if (GetFileAttributes(filePath) != INVALID_FILE_ATTRIBUTES)
				SendMessage(hWnd, BFFM_ENABLEOK, 0, TRUE);
			else
				SendMessage(hWnd, BFFM_ENABLEOK, 0, TRUE);
		}
		break;
	}

	return 0;
}

void mm_handle_mod_directory_found(HWND hWnd, TCHAR* filePath)
{
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	char* extension = NULL;
	size_t len = 0;

	// find all files in the folder
	_tcscat(filePath, "/*");

	HANDLE hFind = FindFirstFile(filePath, &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		// while we find files, go through them
		do
		{
			len = strlen(findData.cFileName);
			if (len > 3)
			{
				// slice the extension and remove it from the file name
				extension = &findData.cFileName[len - 3];
				findData.cFileName[len - 4] = '\0';

				// check this is a .zip file
				if (strcmp(extension, "zip") != 0)
					continue;

				// add it to the list
				LVITEM listviewItem = { 0 };
				listviewItem.iItem = 0;
				listviewItem.iSubItem = 0;

				ListView_InsertItem(mm_mod_file_list, &listviewItem);

				// and now we need to use setitem for the other columns
				listviewItem.mask = LVIF_TEXT;
				listviewItem.pszText = findData.cFileName;
				listviewItem.iItem = 0;
				listviewItem.iSubItem = 1;
				
				ListView_SetItem(mm_mod_file_list, &listviewItem);

			}
		} while (FindNextFile(hFind, &findData));

		// close the handle
		FindClose(hFind);
	}
#endif
}

