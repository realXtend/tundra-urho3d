md assets\Data
md assets\Scenes
md assets\Assets
xcopy ..\..\Bin\Data\*.* assets\Data /S /E /C /Y
xcopy ..\..\Bin\Scenes\*.* assets\Scenes /S /E /C /Y
xcopy ..\..\Bin\Assets\*.* assets\Assets /S /E /C /Y
xcopy ..\..\Bin\*.json assets /S /E /C /Y
rd /S /Q assets\Data\Shaders\HLSL
