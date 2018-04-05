@if not '%VS120COMNTOOLS%' == '' (
	set BUILD="%VS120COMNTOOLS%/../IDE/devenv.exe"
) else (
	set BUILD="%VS140COMNTOOLS%/../IDE/devenv.exe"
)

@if '%VS120COMNTOOLS%' == '' (
	%BUILD% server.sln /Upgrade
	rmdir /s /q Backup
	del /q UpgradeLog*.htm
)

@%BUILD% server.sln /build "Debug|Win32" /project core
@%BUILD% server.sln /build "Debug|Win32" /project client
@%BUILD% server.sln /build "Debug|Win32" /project server