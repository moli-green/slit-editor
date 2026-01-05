# Makefile for slit

# --- 1. 変数定義 (カスタマイズ可能) ---
# CC: コンパイラ (gcc, clang など)
CC = gcc

# CFLAGS: コンパイルオプション
# -Wall: 警告を全部出す (推奨)
# -O2: 最適化レベル2 (配布用)
CFLAGS = -Wall -O2

# TARGET: 出力する実行ファイル名
TARGET = slit

# PREFIX: インストール先 (標準は /usr/local)
PREFIX = /usr/local

# --- 2. ビルドターゲット ---

# 'make' とだけ打った時に実行されるターゲット
all: $(TARGET)

# コンパイル手順
$(TARGET): slit.c
	$(CC) $(CFLAGS) -o $(TARGET) slit.c

# 'make clean' で生成物を削除
clean:
	rm -f $(TARGET)

# 'make install' でシステムにインストール
install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin..."
	mkdir -p $(PREFIX)/bin
	cp $(TARGET) $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/$(TARGET)
	@echo "Done."

# 'make uninstall' で削除
uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

# PHONY: 実在するファイル名とターゲット名が被った時の誤動作防止
.PHONY: all clean install uninstall

