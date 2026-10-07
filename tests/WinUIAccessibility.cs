using System;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Automation;

public static class WinUIAccessibility {
 [DllImport("user32.dll")] static extern bool SetThreadDesktop(IntPtr desktop);
 static void OnDesktop(IntPtr desktop, Action action) {
  Exception failure=null;
  var worker=new Thread(delegate() {try{SetThreadDesktop(desktop);action();}catch(Exception ex){failure=ex;}});
  worker.IsBackground=true;worker.SetApartmentState(ApartmentState.STA);worker.Start();
  if(!worker.Join(10000))throw new TimeoutException("UI Automation did not respond");
  if(failure!=null)throw failure;
 }
 static AutomationElement Find(IntPtr window,string id) {
  var root=AutomationElement.FromHandle(window);
  var element=root.FindFirst(TreeScope.Descendants,new PropertyCondition(AutomationElement.AutomationIdProperty,id));
  if(element==null)throw new InvalidOperationException("Control not found: "+id);
  return element;
 }
 public static void Invoke(IntPtr window,IntPtr desktop,string id) {
  OnDesktop(desktop,delegate(){((InvokePattern)Find(window,id).GetCurrentPattern(InvokePattern.Pattern)).Invoke();});
 }
 public static void SetRange(IntPtr window,IntPtr desktop,string id,double value) {
  OnDesktop(desktop,delegate(){((RangeValuePattern)Find(window,id).GetCurrentPattern(RangeValuePattern.Pattern)).SetValue(value);});
 }
 public static double RangeValue(IntPtr window,IntPtr desktop,string id) {
  double value=0;OnDesktop(desktop,delegate(){value=((RangeValuePattern)Find(window,id).GetCurrentPattern(RangeValuePattern.Pattern)).Current.Value;});return value;
 }
 public static void SetRangeFraction(IntPtr window,IntPtr desktop,string id,double fraction) {
  OnDesktop(desktop,delegate(){var range=(RangeValuePattern)Find(window,id).GetCurrentPattern(RangeValuePattern.Pattern);range.SetValue(range.Current.Minimum+(range.Current.Maximum-range.Current.Minimum)*fraction);});
 }
 public static void Toggle(IntPtr window,IntPtr desktop,string id) {
  OnDesktop(desktop,delegate(){((TogglePattern)Find(window,id).GetCurrentPattern(TogglePattern.Pattern)).Toggle();});
 }
 public static void SetValue(IntPtr window,IntPtr desktop,string id,string value) {
  OnDesktop(desktop,delegate(){((ValuePattern)Find(window,id).GetCurrentPattern(ValuePattern.Pattern)).SetValue(value);});
 }
 public static void SelectIndex(IntPtr window,IntPtr desktop,string id,int index) {
  OnDesktop(desktop,delegate(){
   var list=Find(window,id);
   var items=list.FindAll(TreeScope.Children,new PropertyCondition(AutomationElement.ControlTypeProperty,ControlType.ListItem));
   if(index<0 || index>=items.Count)throw new ArgumentOutOfRangeException("index");
   ((SelectionItemPattern)items[index].GetCurrentPattern(SelectionItemPattern.Pattern)).Select();
  });
 }
 public static void InvokeItemChild(IntPtr window,IntPtr desktop,string listId,int index,string childId) {
  OnDesktop(desktop,delegate(){
   var list=Find(window,listId);
   var items=list.FindAll(TreeScope.Children,new PropertyCondition(AutomationElement.ControlTypeProperty,ControlType.ListItem));
   var child=items[index].FindFirst(TreeScope.Descendants,new PropertyCondition(AutomationElement.AutomationIdProperty,childId));
   ((InvokePattern)child.GetCurrentPattern(InvokePattern.Pattern)).Invoke();
  });
 }
 public static bool Exists(IntPtr window,IntPtr desktop,string id) {
  bool found=false;OnDesktop(desktop,delegate(){var root=AutomationElement.FromHandle(window);found=root.FindFirst(TreeScope.Descendants,new PropertyCondition(AutomationElement.AutomationIdProperty,id))!=null;});return found;
 }
 public static string Name(IntPtr window,IntPtr desktop,string id) {
  string name=null;OnDesktop(desktop,delegate(){name=Find(window,id).Current.Name;});return name;
 }
 public static bool Enabled(IntPtr window,IntPtr desktop,string id) {
  bool enabled=false;OnDesktop(desktop,delegate(){enabled=Find(window,id).Current.IsEnabled;});return enabled;
 }
 public static string Read(IntPtr window, IntPtr desktop) {
  var result=new StringBuilder(); Exception failure=null;
  var worker=new Thread(delegate() {
   try {
    SetThreadDesktop(desktop);
    var root=AutomationElement.FromHandle(window);
    var nodes=root.FindAll(TreeScope.Descendants,Condition.TrueCondition);
    foreach(AutomationElement node in nodes) {
     var c=node.Current;
     if(!String.IsNullOrEmpty(c.AutomationId) || c.ControlType==ControlType.Button)
      result.AppendLine(c.AutomationId+" | "+c.Name+" | "+(c.ControlType==null?"unknown":c.ControlType.ProgrammaticName)+" | enabled="+c.IsEnabled+" | focusable="+c.IsKeyboardFocusable);
    }
   } catch(Exception ex){failure=ex;}
  });
  worker.IsBackground=true;worker.SetApartmentState(ApartmentState.STA);worker.Start();
  if(!worker.Join(10000))throw new TimeoutException("UI Automation did not respond");
  if(failure!=null)throw failure;
  return result.ToString();
 }
}
