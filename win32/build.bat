@if not "%VS120COMNTOOLS%" == "" (
	set BUILD="%VS120COMNTOOLS%/../IDE/devenv.exe"
) else (
	set BUILD="%VS140COMNTOOLS%/../IDE/devenv.exe"
)

@if "%VS120COMNTOOLS%" == "" (
	%BUILD% server.sln /Upgrade
	rmdir /s /q Backup
	del /q UpgradeLog*.htm
)

@%BUILD% server.sln /build "Debug|Win32" /project core /Out core.log
@%BUILD% server.sln /build "Debug|Win32" /project client /Out client.log
@%BUILD% server.sln /build "Debug|Win32" /project server /Out server.log