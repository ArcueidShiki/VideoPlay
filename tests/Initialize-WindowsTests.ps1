# Use the caller's policy-permitted PowerShell host; never change execution policy.
Add-Type -AssemblyName UIAutomationClient,UIAutomationTypes,WindowsBase
if ($PSEdition -eq 'Core') {
 $refs=@(Get-ChildItem -LiteralPath "$PSHOME\ref" -Filter '*.dll' | Where-Object Name -notin @('System.Drawing.dll','WindowsBase.dll') | ForEach-Object FullName)
 $refs+=@([System.Drawing.Bitmap].Assembly.Location,[System.Windows.Automation.AutomationElement].Assembly.Location,[System.Windows.Automation.AutomationIdentifier].Assembly.Location,[System.Windows.Rect].Assembly.Location)
 $refs+=@(Get-ChildItem -LiteralPath $PSHOME -Filter 'System.Private.Windows.*.dll' | ForEach-Object FullName)
 Add-Type -Path "$PSScriptRoot\PlayerWindows.cs","$PSScriptRoot\WinUIAccessibility.cs" -ReferencedAssemblies $refs
} else {
 Add-Type -Path "$PSScriptRoot\PlayerWindows.cs" -ReferencedAssemblies System.Drawing
 Add-Type -Path "$PSScriptRoot\WinUIAccessibility.cs" -ReferencedAssemblies @([System.Windows.Automation.AutomationElement].Assembly.Location,[System.Windows.Automation.AutomationIdentifier].Assembly.Location,[System.Windows.Rect].Assembly.Location)
}
