using System;
using System.Text;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

public static class PlayerWindows {
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    struct STARTUPINFO {
        public int cb; public string lpReserved, lpDesktop, lpTitle;
        public int dwX,dwY,dwXSize,dwYSize,dwXCountChars,dwYCountChars,dwFillAttribute,dwFlags;
        public short wShowWindow, cbReserved2;
        public IntPtr lpReserved2,hStdInput,hStdOutput,hStdError;
    }
    [StructLayout(LayoutKind.Sequential)] struct PROCESS_INFORMATION { public IntPtr process,thread; public uint processId,threadId; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct GUITHREADINFO {
        public int cbSize, flags; public IntPtr active,focus,capture,menuOwner,moveSize,caret; public RECT caretRect;
    }
    public delegate bool EnumProc(IntPtr hwnd, IntPtr state);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
    static extern IntPtr CreateDesktop(string name,IntPtr device,IntPtr mode,int flags,uint access,IntPtr attributes);
    [DllImport("user32.dll",SetLastError=true)] static extern IntPtr OpenInputDesktop(uint flags,bool inherit,uint access);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool GetUserObjectInformation(IntPtr handle,int index,StringBuilder text,int length,out int needed);
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
    static extern bool CreateProcess(string app,StringBuilder command,IntPtr pa,IntPtr ta,bool inherit,uint flags,IntPtr env,string cwd,ref STARTUPINFO startup,out PROCESS_INFORMATION process);
    [DllImport("user32.dll")] static extern bool EnumDesktopWindows(IntPtr desktop,EnumProc callback,IntPtr state);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr window,EnumProc callback,IntPtr state);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr hwnd,StringBuilder text,int count);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr hwnd,StringBuilder text,int count);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint id);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr hwnd,int id);
    [DllImport("user32.dll")] public static extern int GetDlgCtrlID(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hwnd,out RECT rect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd,out RECT rect);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hwnd,int x,int y,int width,int height,bool repaint);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hwnd,int message,IntPtr wParam,IntPtr lParam);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr hwnd,int message,IntPtr wParam,string lParam);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern IntPtr SendMessage(IntPtr hwnd,int message,IntPtr wParam,StringBuilder lParam);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd,int message,IntPtr wParam,IntPtr lParam);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr hwnd,IntPtr dc,uint flags);
    [DllImport("user32.dll")] static extern bool SetThreadDesktop(IntPtr desktop);
    [DllImport("user32.dll",SetLastError=true)] public static extern bool GetGUIThreadInfo(uint thread,ref GUITHREADINFO info);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr handle,uint milliseconds);
    [DllImport("kernel32.dll")] static extern bool GetExitCodeProcess(IntPtr process,out uint code);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
    [DllImport("kernel32.dll")] static extern bool TerminateProcess(IntPtr handle,uint code);
    [DllImport("user32.dll")] static extern bool CloseDesktop(IntPtr desktop);
    public static IntPtr Desktop;
    static string desktopName="VideoPlayTests-"+Guid.NewGuid().ToString("N");
    public static string DesktopName { get { return desktopName; } }
    // Opt-in only, for coordinated tests of the explicitly launched player.
    public static void UseVisibleDesktop() {
        if(Desktop!=IntPtr.Zero)throw new InvalidOperationException("Select desktop before launching test players.");
        Desktop=OpenInputDesktop(0,false,0x01ff);
        if(Desktop==IntPtr.Zero)throw new Exception("OpenInputDesktop: "+Marshal.GetLastWin32Error());
        var name=new StringBuilder(256);int needed;
        if(!GetUserObjectInformation(Desktop,2,name,name.Capacity*2,out needed))throw new Exception("Cannot identify input desktop.");
        desktopName=name.ToString();
    }
    static List<PROCESS_INFORMATION> processes = new List<PROCESS_INFORMATION>();
    public static uint Launch(string exe) { return Launch(exe,"",Path.GetDirectoryName(exe)); }
    public static uint Launch(string exe,string arguments,string workingDirectory) {
        if (Desktop == IntPtr.Zero) {
            Desktop=CreateDesktop(desktopName,IntPtr.Zero,IntPtr.Zero,0,0x01ff,IntPtr.Zero);
            if(Desktop==IntPtr.Zero) throw new Exception("CreateDesktop: "+Marshal.GetLastWin32Error());
        }
        STARTUPINFO startup=new STARTUPINFO(); startup.cb=Marshal.SizeOf(startup);
        startup.lpDesktop=desktopName; startup.dwFlags=1; startup.wShowWindow=1;
        PROCESS_INFORMATION process;
        if(!CreateProcess(exe,new StringBuilder("\""+exe+"\" "+arguments),IntPtr.Zero,IntPtr.Zero,false,0,IntPtr.Zero,workingDirectory,ref startup,out process))
            throw new Exception("CreateProcess: "+Marshal.GetLastWin32Error());
        processes.Add(process); return process.processId;
    }
    public static string Text(IntPtr hwnd) { var s=new StringBuilder(8192); GetWindowText(hwnd,s,s.Capacity); return s.ToString(); }
    public static string Class(IntPtr hwnd) { var s=new StringBuilder(256); GetClassName(hwnd,s,s.Capacity); return s.ToString(); }
    public static IntPtr[] Windows(uint processId) {
        var result=new List<IntPtr>();
        EnumDesktopWindows(Desktop,delegate(IntPtr h,IntPtr _) {uint id; GetWindowThreadProcessId(h,out id); if(id==processId && IsWindowVisible(h)) result.Add(h); return true;},IntPtr.Zero);
        return result.ToArray();
    }
    public static IntPtr[] Children(IntPtr window) {
        var result=new List<IntPtr>();
        EnumChildWindows(window,delegate(IntPtr h,IntPtr _) { result.Add(h); return true;},IntPtr.Zero);
        return result.ToArray();
    }
    public static IntPtr Find(uint processId,string title,int timeout) {
        for(int elapsed=0;elapsed<timeout;elapsed+=100) {
            foreach(var h in Windows(processId)) if(Text(h)==title) return h;
            Thread.Sleep(100);
        }
        var details=new StringBuilder();
        foreach(var p in processes) if(p.processId==processId) {
            uint code;GetExitCodeProcess(p.process,out code);
            details.Append(" Process "+processId+" exit/status "+code+".");
        }
        foreach(var h in Windows(processId)) details.Append(" Window: "+Class(h)+" / "+Text(h));
        throw new Exception("Window not found: "+title+details.ToString());
    }
    public static void Command(IntPtr hwnd,int id) { PostMessage(hwnd,0x111,new IntPtr(id),IntPtr.Zero); }
    public static void Click(IntPtr hwnd) { PostMessage(hwnd,0x00f5,IntPtr.Zero,IntPtr.Zero); }
    public static void SetText(IntPtr hwnd,string text) { SendMessage(hwnd,0x000c,IntPtr.Zero,text); }
    public static string ReadControlText(IntPtr hwnd) { var s=new StringBuilder(8192); SendMessage(hwnd,0x000d,new IntPtr(s.Capacity),s); return s.ToString(); }
    public static void Key(IntPtr hwnd,int key) { PostMessage(hwnd,0x100,new IntPtr(key),IntPtr.Zero); PostMessage(hwnd,0x101,new IntPtr(key),new IntPtr(0x40000000)); }
    public static void Focus(IntPtr dialog,IntPtr control) { SendMessage(dialog,0x0028,control,new IntPtr(1)); }
    public static IntPtr Focused(IntPtr hwnd) {
        IntPtr focus=IntPtr.Zero;
        var reader=new Thread(delegate() {
            SetThreadDesktop(Desktop);
            uint id; uint thread=GetWindowThreadProcessId(hwnd,out id);
            var info=new GUITHREADINFO();info.cbSize=Marshal.SizeOf(info);
            GetGUIThreadInfo(thread,ref info); focus=info.focus;
        });
        reader.Start();reader.Join();return focus;
    }
    public static void Screenshot(IntPtr hwnd,string path) {
        RECT r; GetWindowRect(hwnd,out r);
        using(var b=new Bitmap(r.Right-r.Left,r.Bottom-r.Top))
        using(var g=Graphics.FromImage(b)) {
            IntPtr dc=g.GetHdc(); bool result=PrintWindow(hwnd,dc,2); g.ReleaseHdc(dc);
            if(!result) throw new Exception("PrintWindow failed");
            b.Save(path,ImageFormat.Png);
        }
    }
    public static void Cleanup() {
        foreach(var p in processes) {
            foreach(var h in Windows(p.processId)) PostMessage(h,0x10,IntPtr.Zero,IntPtr.Zero);
            if(WaitForSingleObject(p.process,3000)!=0) TerminateProcess(p.process,1);
            CloseHandle(p.thread); CloseHandle(p.process);
        }
        processes.Clear();
        if(Desktop!=IntPtr.Zero) {CloseDesktop(Desktop); Desktop=IntPtr.Zero;}
    }
}
