INCLUDE_PATH=include/
OBJPATH=obj/
BIN=bin/

obj=$(patsubst %.h, $(OBJPATH)%.o, server.h proxy.h)

all:$(BIN)server $(BIN)client

$(BIN)server:server_main.c $(obj)
	gcc $^ -o $@ -I$(INCLUDE_PATH) -lpthread

$(BIN)client:client.c $(obj)
	gcc $^ -o $@ -I$(INCLUDE_PATH) -lpthread

$(OBJPATH)%.o:%.c
	gcc -c $< -o $@ -I$(INCLUDE_PATH)

clean:
	rm -rf $(obj) $(BIN)*