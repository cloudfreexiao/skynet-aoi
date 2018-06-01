LUA_CLIB_PATH ?= ./

CAOI_SO = $(LUA_CLIB_PATH)/caoi.so
LAOI_SO = $(LUA_CLIB_PATH)/laoi.so

all: $(CAOI_SO) $(LAOI_SO)

$(CAOI_SO): service_aoi.c
		$(CC) $(LFLAGS) -o $@ $(CFLAGS) $< -I../skynet/skynet-src

$(LAOI_SO): lua-aoi.c 
	gcc -O2 -Wall $^ -fPIC --shared -o $@ -Iluafilesystem/src -I../skynet/3rd/lua