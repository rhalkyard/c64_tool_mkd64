bootblk_MODULES:= module bootalloc
bootblk_LIBTYPE:= plugin
bootblk_INSTALLDIRNAME:= plugin
bootblk_DEPS:= mkd64
bootblk_win32_LIBS:= mkd64
bootblk_win32_CFLAGS:= -D__USE_MINGW_ANSI_STDIO=1
$(call librules, bootblk)
