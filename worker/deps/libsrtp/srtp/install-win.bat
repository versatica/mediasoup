:: Installs from srtp windows build directory to directory specified on
:: command line


@if "%1"=="" (
	echo "Usage: %~nx0 destdir"
	exit /b 1
) else (
	set destdir=%1
)

@if not exist %destdir% (
   echo %destdir% not found
   exit /b 1
)

@for %%d in (include\srtp.h crypto\include\crypto.h Debug\srtp.lib Release\srtp.lib) do (
	if not exist "%%d" (
	   echo "%%d not found: are you in the right directory?"
	   exit /b 1
	)
)

mkdir %destdir%\include
mkdir %destdir%\include\srtp
mkdir %destdir%\lib

copy include\*.h %destdir%\include\srtp
copy crypto\include\*.h %destdir%\include\srtp
copy Release\srtp.lib %destdir%\lib\srtp.lib
copy Debug\srtp.lib %destdir%\lib\srtpd.lib
