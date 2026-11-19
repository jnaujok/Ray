// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define NO_WARN_MBCS_MFC_DEPRECATION

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#include <afxcview.h>       // MFC Common Control derived views

    // Networking comes from IASLib (winsock2), not afxsock.h, which would
    // pull in the older winsock.h that cannot be mixed with it.
#include <afxpriv.h>        // MFC Private definitions (for docking tree control)
#include <afxtempl.h>       // MFC Template collection

#include <afxmt.h>          // MFC Multithreaded synchonization classes

#include "RayDoc.h"

#include "Objects.h"
#include "Globals.h"
