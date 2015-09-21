/*
This file is part of the NppCrypt Plugin [www.cerberus-design.de] for Notepad++ [ Copyright (C)2003 Don HO <don.h@free.fr> ]

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "resource.h"
#include "dlg_hash.h"
#include "preferences.h"

DlgHash::DlgHash() : Window(), no_ascii(false)
{}

DlgHash::~DlgHash()
{}

void DlgHash::init(HINSTANCE hInst, HWND parent, crypt::Options::Hash* opt)
{
	Window::init(hInst, parent);
	options = opt;
};

bool DlgHash::doDialog(bool no_ascii)
{
	if(!options)
		return false;
	this->no_ascii = no_ascii;
	if(DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_HASH), _hParent,  (DLGPROC)dlgProc, (LPARAM)this)==IDC_OK)
		return true;
	return false;
}

BOOL CALLBACK DlgHash::dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	switch (Message) 
	{
		case WM_INITDIALOG :
		{
			DlgHash *pHashDialog = (DlgHash *)(lParam);
			pHashDialog->_hSelf = hWnd;
			::SetWindowLongPtr(hWnd, GWL_USERDATA, (long)lParam);
			pHashDialog->run_dlgProc(Message, wParam, lParam);
			return TRUE;
		}

		default :
		{
			DlgHash *pHashDialog = reinterpret_cast<DlgHash *>(::GetWindowLong(hWnd, GWL_USERDATA));
			if (!pHashDialog)
				return FALSE;
			return pHashDialog->run_dlgProc(Message, wParam, lParam);
		}

	}
	return FALSE;
}

BOOL CALLBACK DlgHash::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
        case WM_INITDIALOG :
		{
			if(no_ascii)
			{
				if(options->encoding == crypt::Encoding::ascii) 
					options->encoding = crypt::Encoding::base16;
				::EnableWindow(::GetDlgItem(_hSelf,IDC_HASH_ENC_ASCII),false);
			}
			::SendDlgItemMessage(_hSelf, IDC_HASH_ENC_ASCII, BM_SETCHECK, (options->encoding == crypt::Encoding::ascii), 0);
			::SendDlgItemMessage(_hSelf, IDC_HASH_ENC_HEX, BM_SETCHECK, (options->encoding == crypt::Encoding::base16), 0);
			::SendDlgItemMessage(_hSelf, IDC_HASH_ENC_BASE64, BM_SETCHECK, (options->encoding == crypt::Encoding::base64), 0);

			crypt::help::Iterator::setup(crypt::help::Iterator::Hash, options->use_key);
			while (crypt::help::Iterator::next()) {
				::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_ADDSTRING, 0, (LPARAM)crypt::help::Iterator::getString());
			}
			::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_SETCURSEL, (int)options->algorithm, 0);
			
			::SendDlgItemMessage(_hSelf, IDC_HASH_USE_KEY, BM_SETCHECK, options->use_key, 0);
			::SendDlgItemMessage(_hSelf, IDC_HASH_KEY, EM_LIMITTEXT, NPPC_HMAC_INPUT_MAX, 0);

			for (size_t i = 0; i < preferences.getKeyNum(); i++) {
				::SendDlgItemMessage(_hSelf, IDC_HASH_KEY, CB_ADDSTRING, 0, (LPARAM)preferences.getKeyLabel(i));
			}			
			if (options->key_id >= 0) {
				::SendDlgItemMessage(_hSelf, IDC_HASH_KEY, CB_SETCURSEL, options->key_id, 0);
			}
			else {
				string tstr;
				#ifdef UNICODE
				Encode::utf8_to_wchar(options->key_input.c_str(), options->key_input.size(), tstr);
				#else
				tstr.assign(options->key_input);
				#endif
				::SetDlgItemText(_hSelf, IDC_HASH_KEY, tstr.c_str());
			}

			::EnableWindow(::GetDlgItem(_hSelf,IDC_HASH_KEY), options->use_key);

			return TRUE;
		}
		case WM_COMMAND : 
	    {
		    switch (LOWORD(wParam))
		    {
				case IDC_OK:
					options->algorithm = (crypt::Hash)::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_GETCURSEL, 0, 0);
					if(::SendDlgItemMessage(_hSelf, IDC_HASH_ENC_ASCII, BM_GETCHECK, 0, 0))
						options->encoding = crypt::Encoding::ascii;
					else if(::SendDlgItemMessage(_hSelf, IDC_HASH_ENC_HEX, BM_GETCHECK, 0, 0))
						options->encoding = crypt::Encoding::base16;
					else
						options->encoding = crypt::Encoding::base64;
					options->use_key = !!::SendDlgItemMessage(_hSelf, IDC_HASH_USE_KEY, BM_GETCHECK, 0, 0);
					options->key_id = ::SendDlgItemMessage(_hSelf, IDC_HASH_KEY, CB_GETCURSEL, 0, 0);

					if(options->use_key)
					{
						if (options->key_id < 0)
						{
							TCHAR temp_key[NPPC_HMAC_INPUT_MAX + 1];
							::GetDlgItemText(_hSelf, IDC_HASH_KEY, temp_key, NPPC_HMAC_INPUT_MAX + 1);
							if (!lstrlen(temp_key))
							{
								::MessageBox(_hSelf, TEXT("Please enter a key."), TEXT("Error"), MB_OK);
								return FALSE;
							}
							try {
								#ifdef UNICODE
								Encode::wchar_to_utf8(temp_key, -1, options->key_input);
								#else
								options->key.assign(temp_key);
								#endif
							}
							catch (CExc& exc) {
								::MessageBox(_hSelf, exc.getErrorMsg(), TEXT("Error"), MB_OK);
								return false;
							}
						}
					}

					EndDialog(_hSelf, IDC_OK);
				    return TRUE;

				case IDC_CANCEL :
				    EndDialog(_hSelf, IDC_CANCEL);
					return TRUE;

				case IDC_HASH_USE_KEY: 
					{
						bool use_key = !!::SendDlgItemMessage(_hSelf, IDC_HASH_USE_KEY, BM_GETCHECK, 0, 0);
						::EnableWindow(::GetDlgItem(_hSelf,IDC_HASH_KEY), use_key);

						int cur_sel = ::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_GETCURSEL, 0, 0);
						int count=0;
						::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_RESETCONTENT, 0, 0);
						crypt::help::Iterator::setup(crypt::help::Iterator::Hash, use_key);
						while (crypt::help::Iterator::next()) {
							::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_ADDSTRING, 0, (LPARAM)crypt::help::Iterator::getString());
							count++;
						}
						if(cur_sel < count)
							::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_SETCURSEL, cur_sel, 0);
						else
							::SendDlgItemMessage(_hSelf, IDC_HASH_ALGO, CB_SETCURSEL, count-1, 0);					
					}
					break;

			    default :
				    break;
		    }
		    break;
	    }
	}
	return FALSE;
}