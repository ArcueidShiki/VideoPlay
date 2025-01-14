# Configuration:

## Architecture : `x86`

## Include Direcotries:

Choose this: `Properties -> VC Directories -> include direcotries (this way is adding to env path)`

Instead of this: `Properties -> C/C++ additional include directories (this way is add compile arguments)`

`../ThirdParty/vlc/include`

`../ThirdParty/vlc/include/vlc;`

`.`

## Link Static Libraries:

`Properties -> Linker -> Input -> addtional dependencies(static Libraries) ->`

`..\ThirdParty\vlc\lib\libvlc.lib`

`..\ThirdParty\vlc\lib\libvlccore.lib`

## Dynamic Libraries:

`Properties -> Debugging -> Environment ->`

`PATH=$(ProjectDir)..\ThirdParty\vlc\;$(PATH)`

## Output

`Properties -> Advanced -> Use of MFC -> Use of MFC in a Shared DLL / static Library`