# 头文件目录
INCLUDE_PATH=include/
# 二进制文件目录
BIN_PATH=bin/
# 链接文件
OBJ_PATH=obj/
# 源文件
SRC_PATH=src/

src=$(wildcard $(SRC_PATH)*.c)
obj=$(patsubst $(SRC_PATH)%.c, $(OBJ_PATH)%.o, $(src))


%:%.c $(obj)
	gcc $^ -o $(BIN_PATH)$@  -I$(INCLUDE_PATH) -lpthread

$(OBJ_PATH)%.o:$(SRC_PATH)%.c
	gcc -c $< -o $@ -I$(INCLUDE_PATH)

obj:$(obj)

re: clean $(obj)

clean:
	rm -f $(obj) $(BIN_PATH)*