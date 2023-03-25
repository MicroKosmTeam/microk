@echo -off
mode 80 25

cls
if exist .\microk.efi then
 .\microk.efi
 goto END
endif

if exist fs0:\microk.efi then
 fs0:
 echo Found bootloader on fs0:
 microk.efi
 goto END
endif

if exist fs1:\microk.efi then
 fs1:
 echo Found bootloader on fs1:
 microk.efi
 goto END
endif

if exist fs2:\microk.efi then
 fs2:
 echo Found bootloader on fs2:
 microk.efi
 goto END
endif

if exist fs3:\microk.efi then
 fs3:
 echo Found bootloader on fs3:
 microk.efi
 goto END
endif

if exist fs4:\microk.efi then
 fs4:
 echo Found bootloader on fs4:
 microk.efi
 goto END
endif

if exist fs5:\microk.efi then
 fs5:
 echo Found bootloader on fs5:
 microk.efi
 goto END
endif

if exist fs6:\microk.efi then
 fs6:
 echo Found bootloader on fs6:
 microk.efi
 goto END
endif

if exist fs7:\microk.efi then
 fs7:
 echo Found bootloader on fs7:
 microk.efi
 goto END
endif

 echo "Unable to find bootloader".

:END
