!define VERSION "1.0.5"
!define APP "lolQt"
!define PUBLISHER "c't"
!define QTPATH "D:\Developer\Qt-5.3.1\5.3\msvc2012_opengl"

Name "${APP} ${VERSION}"
OutFile "${APP}-${VERSION}-setup.exe"
InstallDir $PROGRAMFILES\${APP}
InstallDirRegKey HKLM "Software\${PUBLISHER}\${APP}" "Install_Dir"
RequestExecutionLevel admin
SetCompressor lzma
ShowInstDetails show

Page license

  LicenseData ..\LICENSE

Page directory

Page instfiles


;Section "Visual Studio 2012 redistributables"
;  File D:\Developer\Qt-5.3\vcredist\vcredist_sp1_x86.exe
;SectionEnd

Section "lolQt"

;  ReadRegStr $1 HKLM "SOFTWARE\Microsoft\VisualStudio\11.0\VC\VCRedist\x86" "Installed"
;  StrCmp $1 1 vcredist_installed
;  DetailPrint "Installing Visual Studio 2010 Redistributable (x86) ..."
;  ExecWait 'vcredist_sp1_x86.exe'
;vcredist_installed:

  SetOutPath $INSTDIR
  CreateDirectory $INSTDIR\plugins
  CreateDirectory $INSTDIR\plugins\imageformats
  CreateDirectory $INSTDIR\platforms
  CreateDirectory $INSTDIR\mediaservice
  CreateDirectory $INSTDIR\sampledata
  File v${VERSION}\lolQt.exe
  File v${VERSION}\lolQt.exe.embed.manifest
  File ${QTPATH}\bin\icudt52.dll
  File ${QTPATH}\bin\icuin52.dll
  File ${QTPATH}\bin\icuuc52.dll
  File ${QTPATH}\bin\Qt5Concurrent.dll
  File ${QTPATH}\bin\Qt5Core.dll
  File ${QTPATH}\bin\Qt5Gui.dll
  File ${QTPATH}\bin\Qt5Multimedia.dll
  File ${QTPATH}\bin\Qt5Network.dll
  File ${QTPATH}\bin\Qt5Widgets.dll
  File ..\LICENSE
  File mencoder.exe
  File big_noodle_titling.ttf
  File /oname=sampledata\thirsty-cat.gif sampledata\thirsty-cat.gif
  File /oname=sampledata\banging-bear.gif sampledata\banging-bear.gif
  File /oname=sampledata\cat-on-treadmill.gif sampledata\cat-on-treadmill.gif
  File /oname=plugins\imageformats\qgif.dll ${QTPATH}\plugins\imageformats\qgif.dll
  File /oname=platforms\qminimal.dll ${QTPATH}\plugins\platforms\qminimal.dll
  File /oname=platforms\qwindows.dll ${QTPATH}\plugins\platforms\qwindows.dll
  File /oname=mediaservice\dsengine.dll ${QTPATH}\plugins\mediaservice\dsengine.dll
  File /oname=mediaservice\qtmedia_audioengine.dll ${QTPATH}\plugins\mediaservice\qtmedia_audioengine.dll
  File /oname=mediaservice\wmfengine.dll ${QTPATH}\plugins\mediaservice\wmfengine.dll
  WriteUninstaller $INSTDIR\uninstall.exe
SectionEnd


Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\${APP}"
  CreateShortCut "$SMPROGRAMS\${APP}\${APP} ${VERSION}.lnk" "$INSTDIR\${APP}.exe"
  CreateShortcut "$SMPROGRAMS\${APP}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd


Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"
  DeleteRegKey HKLM SOFTWARE\${APP}

  Delete $INSTDIR\lolQt.exe
  Delete $INSTDIR\lolQt.exe.embed.manifest
  Delete $INSTDIR\LICENSE
  Delete $INSTDIR\mencoder.exe
  Delete $INSTDIR\big_noodle_titling.ttf
  Delete $INSTDIR\icudt52.dll
  Delete $INSTDIR\icuin52.dll
  Delete $INSTDIR\icuuc52.dll
  Delete $INSTDIR\Qt5Concurrent.dll
  Delete $INSTDIR\Qt5Core.dll
  Delete $INSTDIR\Qt5Gui.dll
  Delete $INSTDIR\Qt5Multimedia.dll
  Delete $INSTDIR\Qt5Network.dll
  Delete $INSTDIR\Qt5Widgets.dll
  Delete $INSTDIR\uninstall.exe

  Delete $INSTDIR\mediaservice\dsengine.dll
  Delete $INSTDIR\mediaservice\qtmedia_audioengine.dll
  Delete $INSTDIR\mediaservice\wmfengine.dll
  RMDir $INSTDIR\mediaservice

  Delete $INSTDIR\platforms\qminimal.dll
  Delete $INSTDIR\platforms\qwindows.dll
  RMDir $INSTDIR\platforms

  Delete $INSTDIR\plugins\imageformats\qgif.dll
  RMDir $INSTDIR\plugins\imageformats

  RMDir $INSTDIR\plugins

  Delete $INSTDIR\sampledata\thirsty-cat.gif
  Delete $INSTDIR\sampledata\banging-bear.gif
  Delete $INSTDIR\sampledata\cat-on-treadmill.gif
  RMDir $INSTDIR\sampledata
  RMDir $INSTDIR

  Delete "$SMPROGRAMS\lolqt\*.*"
  RMDir "$SMPROGRAMS\lolqt"
  RMDir "$INSTDIR"
SectionEnd
