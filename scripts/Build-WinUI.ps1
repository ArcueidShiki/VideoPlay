param(
 [string]$Dotnet='dotnet',
 [string]$PublishDirectory='',
 [string]$PackageCache='',
 [string]$RuntimeCache='',
 [string]$InnoCompiler='',
 [switch]$SkipRestore
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot
if(!$PublishDirectory){$PublishDirectory=Join-Path $repo 'dist\win-x64'}
if(!$PackageCache){$PackageCache=Join-Path $repo '.build\nuget'}
if(!$RuntimeCache){$RuntimeCache=Join-Path $repo '.build\vc-runtime'}
$PublishDirectory=[IO.Path]::GetFullPath($PublishDirectory)
$env:NUGET_PACKAGES=[IO.Path]::GetFullPath($PackageCache)
$env:DOTNET_CLI_TELEMETRY_OPTOUT='1'
$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE='1'
$env:DOTNET_GENERATE_ASPNET_CERTIFICATE='false'
$env:DOTNET_CLI_HOME=Join-Path $repo '.build\dotnet-home'
$env:DOTNET_ROOT=Split-Path (Get-Command $Dotnet).Source
$env:DOTNET_ROLL_FORWARD='Major'
$project=Join-Path $repo 'WinUIPlayer\VideoPlay.WinUI.csproj'
if(!$SkipRestore) {
 & $Dotnet restore $project -p:Platform=x64 --configfile (Join-Path $repo 'WinUIPlayer\NuGet.Config')
 if($LASTEXITCODE){throw 'NuGet restore failed'}
}
& $Dotnet publish $project -c Release -p:Platform=x64 --no-restore -o $PublishDirectory
if($LASTEXITCODE){throw 'Publish failed'}

# Official VS 2022 redist package, pinned to Microsoft's SHA256 from its catalog.
# Extract only: no system runtime installation, registry change or admin rights.
$hash='4aaf54db0bfc9435f7c3660e1a00237a4b556042bfeea64bde44c2e0194e6ee5'
$url='https://download.visualstudio.microsoft.com/download/pr/45d3b8dd-bced-4b37-9974-142f748d710c/'+$hash+'/Microsoft.VC.14.44.17.14.CRT.Redist.X64.base.vsix'
$crt=Join-Path $RuntimeCache 'Contents\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT'
if(!(Test-Path (Join-Path $crt 'vcruntime140.dll'))) {
 New-Item -ItemType Directory $RuntimeCache -Force | Out-Null
 $archive=Join-Path $RuntimeCache 'runtime.zip'
 Invoke-WebRequest $url -OutFile $archive -UseBasicParsing
 if((Get-FileHash $archive -Algorithm SHA256).Hash -ne $hash){throw 'Microsoft runtime archive checksum mismatch'}
 Expand-Archive $archive $RuntimeCache -Force
}
Copy-Item (Join-Path $crt '*.dll') $PublishDirectory -Force
Copy-Item (Join-Path $repo 'LICENSE') (Join-Path $PublishDirectory 'LICENSE.txt') -Force
$licenseDir=Join-Path $PublishDirectory 'licenses'
New-Item -ItemType Directory $licenseDir -Force | Out-Null
foreach($package in @('microsoft.windowsappsdk','microsoft.windowsappsdk.winui','microsoft.netcore.app.runtime.win-x64')) {
 $version=Get-ChildItem (Join-Path $env:NUGET_PACKAGES $package) -Directory | Sort-Object Name -Descending | Select-Object -First 1
 if(!$version){throw "Missing package license source: $package"}
 foreach($name in @('LICENSE.TXT','NOTICE.txt','THIRD-PARTY-NOTICES.TXT')) {
  $notice=Join-Path $version.FullName $name
  if(Test-Path $notice){Copy-Item $notice (Join-Path $licenseDir "$package-$name") -Force}
 }
}
foreach($file in @('VideoPlay.exe','VideoPlay.pri','MainWindow.xbf','FFmpegInteropX.dll','avcodec-62.dll','vcruntime140.dll','msvcp140.dll')) {
 if(!(Test-Path (Join-Path $PublishDirectory $file))){throw "Incomplete publish layout: $file"}
}
if($InnoCompiler) {
 $output=Split-Path $PublishDirectory
 & $InnoCompiler "/DPublishDir=$PublishDirectory" "/DOutputDir=$output" (Join-Path $repo 'installer\VideoPlay.iss')
 if($LASTEXITCODE){throw 'Installer compilation failed'}
}
Write-Host "Reviewable application: $PublishDirectory\VideoPlay.exe"
