#include "cvimext.h"

#ifdef __BORLANDC__
# include <dir.h>
# ifndef _strnicmp
#  define _strnicmp(a, b, c) strnicmp((a), (b), (c))
# endif
#else
static char *searchpath(char *name);
#endif

FORMATETC fmte = {CF_HDROP,
		  (DVTARGETDEVICE FAR *)NULL,
		  DVASPECT_CONTENT,
		  -1,
		  TYMED_HGLOBAL
		 };
STGMEDIUM medium;
HRESULT hres = 0;
UINT cbFiles = 0;

#define BUFSIZE 1100

    static void
getCvimName(char *name, int runtime)
{
    HKEY	keyhandle;
    DWORD	hlen;

    name[0] = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\cVim\\Cvim", 0,
				       KEY_READ, &keyhandle) == ERROR_SUCCESS)
    {
	hlen = BUFSIZE;
	if (RegQueryValueEx(keyhandle, "path", 0, NULL, (BYTE *)name, &hlen)
							     != ERROR_SUCCESS)
	    name[0] = 0;
	else
	    name[hlen] = 0;
	RegCloseKey(keyhandle);
    }

    if (name[0] == 0)
	strcpy(name, searchpath("cvim.exe"));

    if (!runtime)
    {
	if (name[0] == 0)
	    strcpy(name, searchpath("cvim.bat"));
	if (name[0] == 0)
	    strcpy(name, "cvim");	

	strcat(name, " --literal");
    }
}

    static void
getRuntimeDir(char *buf)
{
    int		idx;

    getCvimName(buf, 1);
    if (buf[0] != 0)
    {
	// When no path found, use the search path to expand it.
	if (strchr(buf, '/') == NULL && strchr(buf, '\\') == NULL)
	    strcpy(buf, searchpath(buf));

	for (idx = strlen(buf) - 1; idx >= 0; idx--)
	    if (buf[idx] == '\\' || buf[idx] == '/')
	    {
		buf[idx + 1] = 0;
		break;
	    }
    }
}

//
// GETTEXT: translated messages and menu entries
//
#ifndef FEAT_GETTEXT
# define _(x)  x
#else
# define _(x)  (*dyn_libintl_gettext)(x)
# define CVIMPACKAGE "cvim"
# ifndef GETTEXT_DLL
#  define GETTEXT_DLL "libintl.dll"
# endif

// Dummy functions
static char *null_libintl_gettext(const char *);
static char *null_libintl_textdomain(const char *);
static char *null_libintl_bindtextdomain(const char *, const char *);
static int dyn_libintl_init(char *dir);
static void dyn_libintl_end(void);

static HINSTANCE hLibintlDLL = 0;
static char *(*dyn_libintl_gettext)(const char *) = null_libintl_gettext;
static char *(*dyn_libintl_textdomain)(const char *) = null_libintl_textdomain;
static char *(*dyn_libintl_bindtextdomain)(const char *, const char *)
						= null_libintl_bindtextdomain;

//
// Attempt to load libintl.dll.  If it doesn't work, use dummy functions.
// "dir" is the directory where the libintl.dll might be.
// Return 1 for success, 0 for failure.
//
    static int
dyn_libintl_init(char *dir)
{
    int		i;
    static struct
    {
	char	    *name;
	FARPROC	    *ptr;
    } libintl_entry[] =
    {
	{"gettext",		(FARPROC*)&dyn_libintl_gettext},
	{"textdomain",		(FARPROC*)&dyn_libintl_textdomain},
	{"bindtextdomain",	(FARPROC*)&dyn_libintl_bindtextdomain},
	{NULL, NULL}
    };

    // No need to initialize twice.
    if (hLibintlDLL)
	return 1;

    strcat(dir, GETTEXT_DLL);
    hLibintlDLL = LoadLibrary(dir);
    if (!hLibintlDLL)
    {
	hLibintlDLL = LoadLibrary(GETTEXT_DLL);
	if (!hLibintlDLL)
	    return 0;
    }

    // Get the addresses of the functions we need.
    for (i = 0; libintl_entry[i].name != NULL
					 && libintl_entry[i].ptr != NULL; ++i)
    {
	if ((*libintl_entry[i].ptr = GetProcAddress(hLibintlDLL,
					      libintl_entry[i].name)) == NULL)
	{
	    dyn_libintl_end();
	    return 0;
	}
    }
    return 1;
}

    static void
dyn_libintl_end(void)
{
    if (hLibintlDLL)
	FreeLibrary(hLibintlDLL);
    hLibintlDLL			= NULL;
    dyn_libintl_gettext		= null_libintl_gettext;
    dyn_libintl_textdomain	= null_libintl_textdomain;
    dyn_libintl_bindtextdomain	= null_libintl_bindtextdomain;
}

    static char *
null_libintl_gettext(const char *msgid)
{
    return (char *)msgid;
}

    static char *
null_libintl_bindtextdomain(const char *domainname, const char *dirname)
{
    return NULL;
}

    static char *
null_libintl_textdomain(const char* domainname)
{
    return NULL;
}

//
// Setup for translating strings.
//
    static void
dyn_gettext_load(void)
{
    char    szBuff[BUFSIZE];
    char    szLang[BUFSIZE];
    DWORD   len;
    HKEY    keyhandle;
    int	    gotlang = 0;

    strcpy(szLang, "LANG=");

    // First try getting the language from the registry, this can be
    // used to overrule the system language.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\cVim\\Cvim", 0,
				       KEY_READ, &keyhandle) == ERROR_SUCCESS)
    {
	len = BUFSIZE;
	if (RegQueryValueEx(keyhandle, "lang", 0, NULL, (BYTE*)szBuff, &len)
							     == ERROR_SUCCESS)
	{
	    szBuff[len] = 0;
	    strcat(szLang, szBuff);
	    gotlang = 1;
	}
	RegCloseKey(keyhandle);
    }

    if (!gotlang && getenv("LANG") == NULL)
    {
	// Get the language from the system.
	// Could use LOCALE_SISO639LANGNAME, but it's not in Win95.
	// LOCALE_SABBREVLANGNAME gives us three letters, like "enu", we use
	// only the first two.
	len = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME,
						    (LPTSTR)szBuff, BUFSIZE);
	if (len >= 2 && _strnicmp(szBuff, "en", 2) != 0)
	{
	    // There are a few exceptions (probably more)
	    if (_strnicmp(szBuff, "cht", 3) == 0
					  || _strnicmp(szBuff, "zht", 3) == 0)
		strcpy(szBuff, "zh_TW");
	    else if (_strnicmp(szBuff, "chs", 3) == 0
					  || _strnicmp(szBuff, "zhc", 3) == 0)
		strcpy(szBuff, "zh_CN");
	    else if (_strnicmp(szBuff, "jp", 2) == 0)
		strcpy(szBuff, "ja");
	    else
		szBuff[2] = 0;	// truncate to two-letter code
	    strcat(szLang, szBuff);
	    gotlang = 1;
	}
    }
    if (gotlang)
	putenv(szLang);

    getRuntimeDir(szBuff);
    if (szBuff[0] != 0)
    {
	len = strlen(szBuff);
	if (dyn_libintl_init(szBuff))
	{
	    strcpy(szBuff + len, "lang");

	    (*dyn_libintl_bindtextdomain)(CVIMPACKAGE, szBuff);
	    (*dyn_libintl_textdomain)(CVIMPACKAGE);
	}
    }
}

    static void
dyn_gettext_free(void)
{
    dyn_libintl_end();
}
#endif // FEAT_GETTEXT

//
// Global variables
//
UINT      g_cRefThisDll = 0;    // Reference count of this DLL.
HINSTANCE g_hmodThisDll = NULL;	// Handle to this DLL itself.


//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
	// Extension DLL one-time initialization
	g_hmodThisDll = hInstance;
	break;

    case DLL_PROCESS_DETACH:
	break;
    }

    return 1;   // ok
}

    static void
inc_cRefThisDLL()
{
#ifdef FEAT_GETTEXT
    if (g_cRefThisDll == 0)
	dyn_gettext_load();
#endif
    InterlockedIncrement((LPLONG)&g_cRefThisDll);
}

    static void
dec_cRefThisDLL()
{
#ifdef FEAT_GETTEXT
    if (InterlockedDecrement((LPLONG)&g_cRefThisDll) == 0)
	dyn_gettext_free();
#else
    InterlockedDecrement((LPLONG)&g_cRefThisDll);
#endif
}

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    if (IsEqualIID(rclsid, CLSID_ShellExtension))
    {
	CShellExtClassFactory *pcf = new CShellExtClassFactory;

	return pcf->QueryInterface(riid, ppvOut);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

CShellExtClassFactory::CShellExtClassFactory()
{
    m_cRef = 0L;

    inc_cRefThisDLL();
}

CShellExtClassFactory::~CShellExtClassFactory()
{
    dec_cRefThisDLL();
}

STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid,
						   LPVOID FAR *ppv)
{
    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
	*ppv = (LPCLASSFACTORY)this;

	AddRef();

	return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
{
    if (InterlockedDecrement((LPLONG)&m_cRef))
	return m_cRef;

    delete this;

    return 0L;
}

STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
						      REFIID riid,
						      LPVOID *ppvObj)
{
    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCSHELLEXT pShellExt = new CShellExt();  //Create the CShellExt object

    if (NULL == pShellExt)
	return E_OUTOFMEMORY;

    return pShellExt->QueryInterface(riid, ppvObj);
}


STDMETHODIMP CShellExtClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
}

// *********************** CShellExt *************************
CShellExt::CShellExt()
{
    m_cRef = 0L;
    m_pDataObj = NULL;

    inc_cRefThisDLL();
}

CShellExt::~CShellExt()
{
    if (m_pDataObj)
	m_pDataObj->Release();

    dec_cRefThisDLL();
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
    {
	*ppv = (LPSHELLEXTINIT)this;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
	*ppv = (LPCONTEXTMENU)this;
    }

    if (*ppv)
    {
	AddRef();

	return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

STDMETHODIMP_(ULONG) CShellExt::Release()
{

    if (InterlockedDecrement((LPLONG)&m_cRef))
	return m_cRef;

    delete this;

    return 0L;
}


STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
				   LPDATAOBJECT pDataObj,
				   HKEY hRegKey)
{
    // Initialize can be called more than once
    if (m_pDataObj)
	m_pDataObj->Release();

    // duplicate the object pointer and registry handle

    if (pDataObj)
    {
	m_pDataObj = pDataObj;
	pDataObj->AddRef();
    }

    return NOERROR;
}


STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
					 UINT indexMenu,
					 UINT idCmdFirst,
					 UINT idCmdLast,
					 UINT uFlags)
{
    UINT idCmd = idCmdFirst;

    hres = m_pDataObj->GetData(&fmte, &medium);
    if (medium.hGlobal)
	cbFiles = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, 0, 0);

    // InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

    // Initialize m_cntOfHWnd to 0
    m_cntOfHWnd = 0;
    EnumWindows(EnumWindowsProc, (LPARAM)this);

    if (cbFiles > 1)
    {
	InsertMenu(hMenu,
		indexMenu++,
		MF_STRING|MF_BYPOSITION,
		idCmd++,
		_("Edit with &multiple cVims"));

	InsertMenu(hMenu,
		indexMenu++,
		MF_STRING|MF_BYPOSITION,
		idCmd++,
		_("Edit with single &cVim"));

	if (cbFiles <= 4)
	{
	    // Can edit up to 4 files in diff mode
	    InsertMenu(hMenu,
		    indexMenu++,
		    MF_STRING|MF_BYPOSITION,
		    idCmd++,
		    _("Diff with cVim"));
	    m_edit_existing_off = 3;
	}
	else
	    m_edit_existing_off = 2;

    }
    else
    {
	InsertMenu(hMenu,
		indexMenu++,
		MF_STRING|MF_BYPOSITION,
		idCmd++,
		_("Edit with &cVim"));
	m_edit_existing_off = 1;
    }

    for (int i = 0; i < m_cntOfHWnd; i++)
    {
	char title[BUFSIZE];
	char temp[BUFSIZE];

	// Obtain window title, continue if can not
	if (GetWindowText(m_hWnd[i], title, BUFSIZE - 1) == 0)
	    continue;
	// Truncate the title before the path, keep the file name
	char *pos = strchr(title, '(');
	if (pos != NULL)
	{
	    if (pos > title && pos[-1] == ' ')
		--pos;
	    *pos = 0;
	}
	// Now concatenate
	strncpy(temp, _("Edit with existing cVim - "), BUFSIZE - 1);
	strncat(temp, title, BUFSIZE - 1);
	InsertMenu(hMenu,
		indexMenu++,
		MF_STRING|MF_BYPOSITION,
		idCmd++,
		temp);
    }
    // InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

    // Must return number of menu items we added.
    return ResultFromShort(idCmd-idCmdFirst);
}


STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr = E_INVALIDARG;
    if (!HIWORD(lpcmi->lpVerb))
    {
	UINT idCmd = LOWORD(lpcmi->lpVerb);

	if (idCmd >= m_edit_existing_off)
	{
	  
	    hr = PushToWindow(lpcmi->hwnd,
		    lpcmi->lpDirectory,
		    lpcmi->lpVerb,
		    lpcmi->lpParameters,
		    lpcmi->nShow,
		    idCmd - m_edit_existing_off);
	}
	else
	{
	    switch (idCmd)
	    {
		case 0:
		    hr = InvokeCvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow);
		    break;
		case 1:
		    hr = InvokeSingleCvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow,
			    0);
		    break;
		case 2:
		    hr = InvokeSingleCvim(lpcmi->hwnd,
			    lpcmi->lpDirectory,
			    lpcmi->lpVerb,
			    lpcmi->lpParameters,
			    lpcmi->nShow,
			    1);
		    break;
	    }
	}
    }
    return hr;
}

STDMETHODIMP CShellExt::PushToWindow(HWND hParent,
				   LPCSTR pszWorkingDir,
				   LPCSTR pszCmd,
				   LPCSTR pszParam,
				   int iShowCmd,
				   int idHWnd)
{
    HWND hWnd = m_hWnd[idHWnd];

    if (IsIconic(hWnd) != 0)
	ShowWindow(hWnd, SW_RESTORE);
    else
	ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);

    PostMessage(hWnd, WM_DROPFILES, (WPARAM)medium.hGlobal, 0);

    return NOERROR;
}

STDMETHODIMP CShellExt::GetCommandString(UINT idCmd,
					 UINT uFlags,
					 UINT FAR *reserved,
					 LPSTR pszName,
					 UINT cchMax)
{
    if (uFlags == GCS_HELPTEXT && cchMax > 35)
	lstrcpy(pszName, _("Edits the selected file(s) with cVim"));

    return NOERROR;
}

BOOL CALLBACK CShellExt::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    char temp[BUFSIZE];

    // First do a bunch of check
    // No invisible window
    if (!IsWindowVisible(hWnd)) return TRUE;
    if (GetClassName(hWnd, temp, sizeof(temp)) == 0)
	return TRUE;
    if (_strnicmp(temp, "cvim", sizeof("cvim")) != 0)
	return TRUE;
    CShellExt *cs = (CShellExt*) lParam;
    if (cs->m_cntOfHWnd >= MAX_HWND) return TRUE;
    cs->m_hWnd[cs->m_cntOfHWnd] = hWnd;
    cs->m_cntOfHWnd ++;

    return TRUE; // continue enumeration (otherwise this would be false)
}

#ifdef WIN32
// This symbol is not defined in older versions of the SDK or Visual C++.

#ifndef VER_PLATFORM_WIN32_WINDOWS
# define VER_PLATFORM_WIN32_WINDOWS 1
#endif

static DWORD g_PlatformId;

//
// Set g_PlatformId to VER_PLATFORM_WIN32_NT (NT) or
// VER_PLATFORM_WIN32_WINDOWS (Win95).
//
    static void
PlatformId(void)
{
    static int done = FALSE;

    if (!done)
    {
	OSVERSIONINFO ovi;

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx(&ovi);

	g_PlatformId = ovi.dwPlatformId;
	done = TRUE;
    }
}

# ifndef __BORLANDC__
    static char *
searchpath(char *name)
{
    static char widename[2 * BUFSIZE];
    static char location[2 * BUFSIZE + 2];

    // There appears to be a bug in FindExecutableA() on Windows NT.
    // Use FindExecutableW() instead...
    PlatformId();
    if (g_PlatformId == VER_PLATFORM_WIN32_NT)
    {
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)name, -1,
		(LPWSTR)widename, BUFSIZE);
	if (FindExecutableW((LPCWSTR)widename, (LPCWSTR)"",
		    (LPWSTR)location) > (HINSTANCE)32)
	{
	    WideCharToMultiByte(CP_ACP, 0, (LPWSTR)location, -1,
		    (LPSTR)widename, 2 * BUFSIZE, NULL, NULL);
	    return widename;
	}
    }
    else
    {
	if (FindExecutableA((LPCTSTR)name, (LPCTSTR)"",
		    (LPTSTR)location) > (HINSTANCE)32)
	    return location;
    }
    return "";
}
# endif
#endif

STDMETHODIMP CShellExt::InvokeCvim(HWND hParent,
				   LPCSTR pszWorkingDir,
				   LPCSTR pszCmd,
				   LPCSTR pszParam,
				   int iShowCmd)
{
    char m_szFileUserClickedOn[BUFSIZE];
    char cmdStr[BUFSIZE];
    UINT i;

    for (i = 0; i < cbFiles; i++)
    {
	DragQueryFile((HDROP)medium.hGlobal,
		i,
		m_szFileUserClickedOn,
		sizeof(m_szFileUserClickedOn));

	getCvimName(cmdStr, 0);
	strcat(cmdStr, " \"");

	if ((strlen(cmdStr) + strlen(m_szFileUserClickedOn) + 2) < BUFSIZE)
	{
	    strcat(cmdStr, m_szFileUserClickedOn);
	    strcat(cmdStr, "\"");

	    STARTUPINFO si;
	    PROCESS_INFORMATION pi;

	    ZeroMemory(&si, sizeof(si));
	    si.cb = sizeof(si);

	    // Start the child process.
	    if (!CreateProcess(NULL,	// No module name (use command line).
			cmdStr,		// Command line.
			NULL,		// Process handle not inheritable.
			NULL,		// Thread handle not inheritable.
			FALSE,		// Set handle inheritance to FALSE.
			0,		// No creation flags.
			NULL,		// Use parent's environment block.
			NULL,		// Use parent's starting directory.
			&si,		// Pointer to STARTUPINFO structure.
			&pi)		// Pointer to PROCESS_INFORMATION structure.
	       )
	    {
		MessageBox(
		    hParent,
		    _("Error creating process: Check if cvim is in your path!"),
		    _("cvimext.dll error"),
		    MB_OK);
	    }
	    else
	    {
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	    }
	}
	else
	{
	    MessageBox(
		hParent,
		_("Path length too long!"),
		_("cvimext.dll error"),
		MB_OK);
	}
    }

    return NOERROR;
}


STDMETHODIMP CShellExt::InvokeSingleCvim(HWND hParent,
				   LPCSTR pszWorkingDir,
				   LPCSTR pszCmd,
				   LPCSTR pszParam,
				   int iShowCmd,
				   int useDiff)
{
    char	m_szFileUserClickedOn[BUFSIZE];
    char	*cmdStr;
    size_t	cmdlen;
    size_t	len;
    UINT i;

    cmdlen = BUFSIZE;
    cmdStr = (char *)malloc(cmdlen);
    getCvimName(cmdStr, 0);
    if (useDiff)
	strcat(cmdStr, " -d");
    for (i = 0; i < cbFiles; i++)
    {
	DragQueryFile((HDROP)medium.hGlobal,
		i,
		m_szFileUserClickedOn,
		sizeof(m_szFileUserClickedOn));

	len = strlen(cmdStr) + strlen(m_szFileUserClickedOn) + 4;
	if (len > cmdlen)
	{
	    cmdlen = len + BUFSIZE;
	    cmdStr = (char *)realloc(cmdStr, cmdlen);
	}
	strcat(cmdStr, " \"");
	strcat(cmdStr, m_szFileUserClickedOn);
	strcat(cmdStr, "\"");
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // Start the child process.
    if (!CreateProcess(NULL,	// No module name (use command line).
		cmdStr,		// Command line.
		NULL,		// Process handle not inheritable.
		NULL,		// Thread handle not inheritable.
		FALSE,		// Set handle inheritance to FALSE.
		0,		// No creation flags.
		NULL,		// Use parent's environment block.
		NULL,		// Use parent's starting directory.
		&si,		// Pointer to STARTUPINFO structure.
		&pi)		// Pointer to PROCESS_INFORMATION structure.
       )
    {
	MessageBox(
	    hParent,
	    _("Error creating process: Check if cvim is in your path!"),
	    _("cvimext.dll error"),
	    MB_OK);
    }
    else
    {
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
    }

    free(cmdStr);

    return NOERROR;
}
