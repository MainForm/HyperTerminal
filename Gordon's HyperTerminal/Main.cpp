#include <Windows.h>
#include <tchar.h>
#include <process.h>
#include <CommCtrl.h>
#include <stdio.h>
#include "ControlsID.h"
#include "resource.h"
#define MAX_STRLEN 1024

typedef struct _ComboName{
	TCHAR * tName;
	INT iNum;
}COMBONAME;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgPortProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgPortSettingProc(HWND, UINT, WPARAM, LPARAM);
void getstr(HANDLE, UCHAR *);
void putstr(HANDLE, UCHAR *);
HWND hWndMain;
WNDPROC OldEditProc;
HINSTANCE g_hInst;
LPCTSTR lpszClassName = TEXT("form");
HWND hEdit;
HWND hComboComm;
HANDLE hRecvComm = 0;
COMMCONFIG ccfg;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow){
	WNDCLASS WndClass;
	MSG Message;
	HWND hWnd;
	DWORD dwCommMask;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClassName;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClassName, TEXT("Gordon's HyperTerminal"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	for (;;){
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
			if (Message.message == WM_QUIT)
				break;

			TranslateMessage(&Message);
			DispatchMessage(&Message);

						if (hRecvComm) {
				GetCommMask(hRecvComm, &dwCommMask);
				switch (dwCommMask) {
				case EV_RXCHAR:
					WndProc(hWndMain, WM_RECVCOMM, NULL, NULL);
					break;
				}
			}
			dwCommMask = 0;
		}
		else {

		}
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	UINT uiLen;
	DWORD dwWriteofBytes, dwReadofBytes;
	UCHAR tRecvChr = 0, tWriteChr = 0;
	UCHAR sRecvStr[3] = "", sWriteStr[3] = "";
	OVERLAPPED ov;
	DWORD dwErr = 0;
	UCHAR cTest = 'c';

	switch (iMessage){
	case WM_CREATE:

		hWndMain = hWnd;
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_BORDER | WS_VISIBLE |
			ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY, 0, 0, 0, 0, hWnd, (HMENU)ID_EDIT_MAIN, g_hInst, NULL);
		OldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG)EditProc);

		if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PORT), hWnd, DlgPortProc)){
			if(DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PORTSETTING), hWnd, DlgPortSettingProc)){
				
			}
		}
		return 0;
	case WM_RECVCOMM:

		dwErr = ReadFile(hRecvComm, &tRecvChr, 1, &dwReadofBytes, NULL);
		if (!dwErr)
			return 0;
		sRecvStr[0] = tRecvChr;
		if (tRecvChr >= 164) {
			ReadFile(hRecvComm, &tRecvChr, 1, &dwReadofBytes, NULL);
			sRecvStr[1] = tRecvChr;
		}
		uiLen = GetWindowTextLength(hEdit);
		SendMessage(hEdit, EM_SETSEL, uiLen, uiLen);
		SendMessage(hEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)sRecvStr);
		return 0;
	case WM_CHAR:
		sprintf((char *)sWriteStr, "%c", (UCHAR)wParam);
		dwErr = WriteFile(hRecvComm, &cTest, 1, &dwWriteofBytes, NULL);
		if (dwErr == 0)
			dwErr = GetLastError();
			return 0;
		if (sWriteStr[0] >= 164) {
			WriteFile(hRecvComm, &sWriteStr[1], 1, &dwWriteofBytes, &ov);
		}
		return 0;
	case WM_SIZE:
		SetWindowPos(hEdit, NULL, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 70, SWP_NOZORDER);
		return 0;
	case WM_DESTROY:
		SetWindowLongPtr(hEdit, GWL_WNDPROC, (LONG)OldEditProc);
		CloseHandle(hRecvComm);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

BOOL CALLBACK DlgPortProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam){
	UINT iID;
	INT i;
	TCHAR tCommName[10];
	COMMTIMEOUTS cto;

	switch (iMessage){
	case WM_INITDIALOG:
		for (i = 0; i < 10; i++){
			wsprintf(tCommName, TEXT("COM%d"), i);
			hRecvComm = CreateFile(tCommName, GENERIC_READ ,
				NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hRecvComm != INVALID_HANDLE_VALUE){
				SendDlgItemMessage(hDlg, IDC_COMBO_COMM, CB_ADDSTRING, NULL, (LPARAM)tCommName);
				CloseHandle(hRecvComm);
				hRecvComm = 0;
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)){
		case IDOK:
			cto.ReadIntervalTimeout = MAXWORD;
			cto.ReadTotalTimeoutConstant = 0;
			cto.ReadTotalTimeoutMultiplier = 0;
			cto.WriteTotalTimeoutConstant = 0;
			cto.WriteTotalTimeoutMultiplier = 0;
			GetDlgItemText(hDlg, IDC_COMBO_COMM, tCommName, 10);
			if (!_tcscmp(tCommName, TEXT(""))){
				MessageBox(hDlg, TEXT("포트을 선택해주세요."), TEXT("확인"), MB_OK);
				break;
			}
			hRecvComm = CreateFile(tCommName, GENERIC_READ | GENERIC_WRITE,
				NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
			SetCommMask(hRecvComm, EV_RXCHAR);
			SetCommTimeouts(hRecvComm, &cto);
			EndDialog(hDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			hWndMain = 0;
			memset(tCommName, NULL, sizeof(tCommName));
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		break;
	}
	
	return FALSE;
}

BOOL CALLBACK DlgPortSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam){
	DWORD dwSizeofccfg, i, dwCTRLID, dwComboIndex, dwCombo;
	TCHAR tStr[32] = "";
	DWORD dwPortSettings[4];
	DWORD dwBaudRate[14] = {
		CBR_110,
		CBR_300,
		CBR_600,
		CBR_1200,
		CBR_2400,
		CBR_4800,
		CBR_9600,
		CBR_14400,
		CBR_19200,
		CBR_38400,
		CBR_57600,
		CBR_115200,
		CBR_128000,
		CBR_256000
	};

	COMBONAME pn[5] = {
		{ TEXT("짝수"), 2 },
		{ TEXT("홀수"), 1 },
		{ TEXT("없음"), 0 },
		{ TEXT("표시"), 3 },
		{ TEXT("공백"), 4 }
	};

	TCHAR *tStopBits[3] = {
		TEXT("1"),
		TEXT("1.5"),
		TEXT("2")
	};

	switch (iMessage){
	case WM_INITDIALOG:

		for (i = 0; i < sizeof(dwBaudRate) / sizeof(DWORD); i++){
			wsprintf(tStr, TEXT("%d"), dwBaudRate[i]);
			SendDlgItemMessage(hDlg, IDC_COMBO_BAUDRATE, CB_ADDSTRING, NULL, (LPARAM)tStr);
			SendDlgItemMessage(hDlg, IDC_COMBO_BAUDRATE, CB_SETITEMDATA, i, dwBaudRate[i]);
		}

		for (i = 4; i <= 8; i++){
			wsprintf(tStr, TEXT("%d"), i);
			SendDlgItemMessage(hDlg, IDC_COMBO_BYTESIZE, CB_ADDSTRING, NULL, (LPARAM)tStr);
			SendDlgItemMessage(hDlg, IDC_COMBO_BYTESIZE, CB_SETITEMDATA, i - 4, i);
		}

		for (i = 0; i < 5; i++){
			SendDlgItemMessage(hDlg, IDC_COMBO_PARITY, CB_ADDSTRING, NULL, (LPARAM)pn[i].tName);
			SendDlgItemMessage(hDlg, IDC_COMBO_PARITY, CB_SETITEMDATA, i, pn[i].iNum);
		}

		for (i = 0; i < 3; i++){
			SendDlgItemMessage(hDlg, IDC_COMBO_STOPBITS, CB_ADDSTRING, NULL, (LPARAM)tStopBits[i]);
			SendDlgItemMessage(hDlg, IDC_COMBO_STOPBITS, CB_SETITEMDATA, i, i);
		}

		ccfg.dwSize = sizeof(COMMCONFIG);
		ccfg.dcb.ByteSize = sizeof(DCB);

		GetCommConfig(hRecvComm, &ccfg, &dwSizeofccfg);

		dwPortSettings[0] = ccfg.dcb.BaudRate;
		dwPortSettings[1] = ccfg.dcb.ByteSize;
		dwPortSettings[2] = ccfg.dcb.Parity;
		dwPortSettings[3] = ccfg.dcb.StopBits;

		for (dwCombo = IDC_COMBO_BAUDRATE; dwCombo <= IDC_COMBO_STOPBITS; dwCombo++){
			for (i = 0; i < SendDlgItemMessage(hDlg, dwCombo, CB_GETCOUNT, NULL, NULL); i++){
				if (dwPortSettings[dwCombo - IDC_COMBO_BAUDRATE] == (DWORD)SendDlgItemMessage(hDlg, dwCombo, CB_GETITEMDATA, i, NULL)){
					SendDlgItemMessage(hDlg, dwCombo, CB_SETCURSEL, i, NULL);
					break;
				}
			}
		}
		
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)){
		case IDOK:
			for (dwCombo = IDC_COMBO_BAUDRATE; dwCombo <= IDC_COMBO_STOPBITS; dwCombo++){
				dwComboIndex = SendDlgItemMessage(hDlg, dwCombo, CB_GETCURSEL, NULL, NULL);
				dwPortSettings[dwCombo - IDC_COMBO_BAUDRATE] = 
					(DWORD)SendDlgItemMessage(hDlg, dwCombo, CB_GETITEMDATA, dwComboIndex, NULL);
			}

			ccfg.dcb.BaudRate = dwPortSettings[0];
			ccfg.dcb.ByteSize = dwPortSettings[1];
			ccfg.dcb.Parity = dwPortSettings[2];
			ccfg.dcb.StopBits = dwPortSettings[3];
			
			SetCommConfig(hRecvComm, &ccfg, sizeof(ccfg));

			EndDialog(hDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return TRUE;
		}
		break;
	}
	
	return FALSE;
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	switch (iMessage){
	case WM_LBUTTONDOWN:
		return 0;
	case WM_CHAR:
		WndProc(hWndMain, WM_CHAR, wParam, lParam);
		break;
	}

	return CallWindowProc(OldEditProc,hWnd,iMessage,wParam,lParam);
}