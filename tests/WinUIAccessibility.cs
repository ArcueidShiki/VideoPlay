using System;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Automation;

public static class WinUIAccessibility {
 [DllImport("user32.dll")] static extern bool SetThreadDesktop(IntPtr desktop);
 public static string Read(IntPtr window, IntPtr desktop) {
  var result=new StringBuilder(); Exception failure=null;
  var worker=new Thread(delegate() {
   try {
    SetThreadDesktop(desktop);
    var root=AutomationElement.FromHandle(window);
    var nodes=root.FindAll(TreeScope.Descendants,Condition.TrueCondition);
    foreach(AutomationElement node in nodes) {
     var c=node.Current;
     if(c.AutomationId.Length>0 || c.ControlType==ControlType.Button)
      result.AppendLine(c.AutomationId+" | "+c.Name+" | "+c.ControlType.ProgrammaticName+" | enabled="+c.IsEnabled+" | focusable="+c.IsKeyboardFocusable);
    }
   } catch(Exception ex){failure=ex;}
  });
  worker.IsBackground=true;worker.SetApartmentState(ApartmentState.STA);worker.Start();
  if(!worker.Join(10000))throw new TimeoutException("UI Automation did not respond");
  if(failure!=null)throw failure;
  return result.ToString();
 }
}
