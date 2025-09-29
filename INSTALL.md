
Chromium Customization Guide

0. Network Management
Determine whether to use a VPN based on your current internet connection quality.

1. Install Prerequisites
Begin by setting up Google’s depot_tools package.

2. Download Source Code
Use gclient sync to fetch the complete Chromium repository. Switch to your desired source directory (e.g., D:\Chromium\src), then advance to Step

3. My personal structure includes:
D:\depot_tools and D:\Chromium\src

4. Configure Build System
Always define build flags via terminal before compiling. Example workflow:

Navigate to D:\Chromium\src
Create an output folder named "newE" (customizable) with:
Basic command: gn gen out/newE
For VS2022 support in Release mode: gn gen out/newE --ide=vs2022 --args="is_debug=false"
Note: Chromium defaults to Multithreaded DLL (MD). To target specific modules like content_shell:
gn gen out/newE --ide=vs2022 --args="is_debug=false" --filters=content/shell:content_shell
(Ensure your uploaded newE_src folder resides here)
4. Compile Project
Build all components and dependencies using:
autoninja -C out/newE (builds entire Chromium)
For individual targets (e.g., content_shell):
autoninja -C out/newE content_shell

5. Edit in Visual Studio
Open out/newE/all.sln in VS2022 to modify C++ files (.cpp, .h) or add functions. Remember to:

Save changes regularly
NEVER compile through Visual Studio
Avoid generating solutions within the IDE
Always trigger builds via Ninja after updating BUILD.gn files.
6. Add Custom Code
Create directories such as D:\Chromium\src\newE_src for your source files, then declare them in corresponding BUILD.gn manifests. Crucially link your static library newE_lib into relevant targets—priority location:
D:\Chromium\src\third_party\blink\renderer\core\BUILD.gn
(Search for existing "newE_lib" references when integrating)

7. Post-Edit Precautions
Refrain from running gclient sync after making code changes, as it may discard unsaved progress.

8. Python Version
The required runtime is python313.dll, found within the extracted newE archive.

9. Runnable Packaging
After successful compilation, rename content_shell.exe to newE.exe in the newly created D:\Chromium\src\out\newE folder. To execute:

Include these alongside the executable:
python_embed folder
newE_py folder
python313.dll library file
(Mirror the structure shown in newE.rar)
Error Recovery Steps
If build fails:

Connect via VPN first, then set these proxies manually:
HTTPS Proxy: http://127.0.0.1:33210 (adjust port if needed)
HTTP Proxy: http://127.0.0.1:33210 (adjust port if needed)
All traffic via SOCKS5: socks5://127.0.0.1:33211 (adjust port if needed)
Set environment variable: DEPOT_TOOLS_WIN_TOOLCHAIN=0
Run hook scripts: gclient runhooks

Retry previous build commands (autoninja or gn gen).
