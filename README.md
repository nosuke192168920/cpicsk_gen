# cpics2_gen

## 概要

cpicsk_genは、
CPicSKのPIC12F509に書き込むプログラムを生成するツールです。

cpicsk_gen.exe を起動すると、
テンプレートとなるプログラムのhexファイル（template.hex）を読み込み、
そこに別のファイル（key.txt）で指定したキー情報を書き込んで、
新規にhexファイル（cpicskprg.hex）を出力します。

## 動作環境

cpicsk_gen.exe は、
32bit版および64bit版のWindows 10上で動作することを確認しています。
それ以外のバージョンのWindows上でも動作するかもしれませんが、未確認です。

実行ファイルが動作しない場合や、
MacやLinuxなどのWindows以外の環境で実行する場合は、
C言語で書かれたソースコードが付属しているので、
これをコンパイルし直して実行ファイルを作成してみてください。

コンパイルの仕方については、
このドキュメントの後ろの方に書かれています。


## 使い方

1. key.txtを、メモ帳などの適当なテキストエディタで開きます

2. key.txtに、Cpicskで書き込みたいキー情報を記述して保存します

      キー情報は、「0xAA,0x55,0x05,...」のように、
      1Byteずつカンマで区切って、「0x」で始まる16進数表記で半角で記述して下さい。

      デフォルトでは、「0xFF」が11Byte分、カンマ区切りで記述されています。

3. cpicsk_gen.exeをダブルクリックするなどして起動します

      正しく実行できた場合、コマンドプロンプトが開き、
      以下のようなメッセージが表示されるはずです。

        Read key file "key.txt" ... OK
        ================================
        Key data:
        00000000  ff ff ff ff ff ff ff ff  ff ff ff
        ================================
        Read template hex file "template.hex" ... OK
        Write to output file "cpicskprg.hex" ... OK
        Press any key to exit.

      `Key data:`と書かれたところの下に、
      key.txt から読み込まれたキー情報が表示されます。

4. ウィンドウをクリックして、何かキーを押すと、ウィンドウが閉じます

      cpicsk_gen.exe と同じフォルダに生成される
      cpicskprg.hex という名前のファイルが、
      PIC12F509に書き込むプログラムになります。

## コンパイルの仕方

gccが使える環境で、makeコマンドを実行してください。

    % make
    gcc -I. -O2 -Wall -c cpicsk_gen.c
    gcc -O2 -Wall cpicsk_gen.o -o cpicsk_gen

Linux環境でコンパイルできることを確認しています。
Macは所持していないため、未確認です。


## 著作権表記

Copyright (c) 2023 nosuke <<sasugaanija@gmail.com>>


## License

MITライセンスに基づきます。


