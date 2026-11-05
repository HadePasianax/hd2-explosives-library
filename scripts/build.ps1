param([string]$Zig='zig',[switch]$Test)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
$compiler=(Get-Command $Zig -ErrorAction Stop).Source
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force $build | Out-Null
$env:ZIG_LIB_DIR=Join-Path (Split-Path -Parent $compiler) 'lib'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'zig-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local'
$env:TEMP=$build;$env:TMP=$build
$flags=@('cc','-target','x86_64-windows-gnu','-O2','-Wall','-Werror','-Wno-unused-parameter','-I',(Join-Path $root 'vendor/hellforge-sdk-16'))
function Compile {
 param([string[]]$Arguments)
 & $compiler @flags @Arguments
 if($LASTEXITCODE -ne 0){throw 'Compilation failed'}
}
Compile -Arguments @('-c','src/library.c','-o',(Join-Path $build 'library.o'))
& $compiler c++ -target x86_64-windows-gnu -O2 -std=c++17 -fno-exceptions -fno-rtti -Wall -Werror -Wno-unused-parameter -I vendor/hellforge-sdk-16 -shared src/inplace_editor.cpp src/json.cpp (Join-Path $build 'library.o') -lbcrypt -o (Join-Path $build 'explosives_library.dll')
if($LASTEXITCODE){throw 'Editor integration compilation failed'}
Compile -Arguments @('-shared','examples/ops-debug/src/ops_debug.c','-o',(Join-Path $build 'explosives_library_ops_debug.dll'))
Compile -Arguments @('-I',(Join-Path $root 'include'),'-x','c','-shared','docs/examples/consumer.c.txt','-o',(Join-Path $build 'example_consumer.dll'))
if($Test){
 & $compiler c++ -target x86_64-windows-gnu -O2 -std=c++17 -fno-exceptions -fno-rtti -Wall -Werror -Wno-unused-parameter -I vendor/hellforge-sdk-16 tests/test-editor-integration.cpp src/json.cpp -lbcrypt -o (Join-Path $build 'test-editor.exe')
 if($LASTEXITCODE){throw 'Editor integration tests did not compile'}
 & (Join-Path $build 'test-editor.exe')
 if($LASTEXITCODE){throw 'Editor integration tests failed'}
 foreach($name in @('test-explosives-library','test-library-ops-debug','test-explosives-library-discovery')){
  $output=Join-Path $build ($name+'.exe')
  Compile -Arguments @(('tests/'+$name+'.c'),'-o',$output)
  if($name -eq 'test-explosives-library-discovery'){
   & $output (Join-Path $build 'explosives_library.dll')
  }else{ & $output }
  if($LASTEXITCODE -ne 0){throw ('Test failed: '+$name)}
 }
}
Write-Output 'Build complete. Outputs and caches are under build/.'
