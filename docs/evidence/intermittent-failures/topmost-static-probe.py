import ctypes as c
from ctypes import wintypes as w
u=c.WinDLL('user32',use_last_error=True)
u.CreateWindowExW.argtypes=[w.DWORD,w.LPCWSTR,w.LPCWSTR,w.DWORD,c.c_int,c.c_int,c.c_int,c.c_int,w.HWND,w.HMENU,w.HINSTANCE,c.c_void_p]
u.CreateWindowExW.restype=w.HWND
u.GetWindowLongPtrW.argtypes=[w.HWND,c.c_int];u.GetWindowLongPtrW.restype=c.c_ssize_t
u.SetWindowPos.argtypes=[w.HWND,w.HWND,c.c_int,c.c_int,c.c_int,c.c_int,w.UINT]
u.ShowWindow.argtypes=[w.HWND,c.c_int]
u.DestroyWindow.argtypes=[w.HWND]
u.SetForegroundWindow.argtypes=[w.HWND]
for visible in (False,True):
    root=u.CreateWindowExW(0,'STATIC','isolated topmost probe',0x80000000,100,100,100,100,None,None,None,None)
    child=u.CreateWindowExW(0,'STATIC','owned probe',0x80000000,100,220,100,100,root,None,None,None)
    if visible: u.ShowWindow(root,4);u.ShowWindow(child,4)
    for name,h in [('owner',root),('owned',child)]:
        c.set_last_error(0)
        result=u.SetWindowPos(h,w.HWND(-1),0,0,0,0,0x13)
        print(visible,name,result,c.get_last_error(),hex(u.GetWindowLongPtrW(h,-20)))
    u.DestroyWindow(root)
