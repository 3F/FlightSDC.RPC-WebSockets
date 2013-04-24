#ifndef RANGE_DLG_H
#define RANGE_DLG_H

#pragma once

#define ATTACH(id, var, txt) \
	var.Attach(GetDlgItem(id)); \
	var.SetWindowText(Text::toT(txt).c_str());

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 512); \
	var = Text::fromT(buf);

class RangeDlg : public CDialogImpl<RangeDlg>
{
		CEdit ctrlName;
		CEdit ctrlStart;
		CEdit ctrlEnd;
		
	public:
		string name, start, end;
		
		enum { IDD = IDD_RANGE_DLG };
		
		BEGIN_MSG_MAP(RangeDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		END_MSG_MAP()
		
		RangeDlg() { };
		
		LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			ctrlName.SetFocus();
			return FALSE;
		}
		
		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
		
			ATTACH(IDC_RANGE_NAME, ctrlName, name);
			ATTACH(IDC_RANGE_START, ctrlStart, start);
			ATTACH(IDC_RANGE_END, ctrlEnd, end);
			
			SetDlgItemText(IDOK, CTSTRING(OK));
			SetDlgItemText(IDCANCEL, CTSTRING(CANCEL));
			SetDlgItemText(IDC_RANGE_DLG_NAME, CTSTRING(IPGUARD_RANGE_DLG_NAME));
			SetDlgItemText(IDC_RANGE_DLG_START, CTSTRING(IPGUARD_RANGE_DLG_START));
			SetDlgItemText(IDC_RANGE_DLG_END, CTSTRING(IPGUARD_RANGE_DLG_END));
			
			CenterWindow(GetParent());
			return FALSE;
		}
		
		LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
		{
			if (wID == IDOK)
			{
				TCHAR buf[512];
				if (ctrlName.GetWindowTextLength() == 0 || ctrlStart.GetWindowTextLength() == 0 || ctrlEnd.GetWindowTextLength() == 0)
				{
					MessageBox(CTSTRING(IPGUATD_VALUES_REQ));
					return 0;
				}
				
				GET_TEXT(IDC_RANGE_NAME, name);
				GET_TEXT(IDC_RANGE_START, start);
				GET_TEXT(IDC_RANGE_END, end);
			}
			EndDialog(wID);
			return 0;
		}
};

#endif // RANGE_DLG_H