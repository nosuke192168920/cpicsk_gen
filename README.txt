■概要

cpicsk_gen および kabuki_config_gen はCPicSKに装着する
PIC12F509に書き込むプログラムを生成するWindows用のツールです。


■使い方

1. GitHubのMAMEのソースコードツリーより、kabuki.cpp をダウンロードして
kabuki_config.exe や cpicsk_gen.exe と同じフォルダに置いてください。

https://github.com/mamedev/mame/blob/master/src/mame/capcom/kabuki.cpp

2. kabuki_code_gen.exe をダブルクリックしてください。
コマンドプロンプトが開き、KABUKI搭載ゲーム基板の一覧が番号と
ともに表示されます。

3. CPicSKを装着する予定のゲーム基板の番号を半角数字で入力して
Enterを押してください。正しく動作している場合、config.txt という
ファイルが生成されます。再度Enterを押すとウィンドウが閉じます。

4. cpicsk_gen.exe をダブルクリックしてください。
再びコマンドプロンプトが開き、cpicskprg.hex というファイルと
cpicskprgx.hexというファイルの2つのファイルが生成されます。
前者が通常のPICライタ用のプログラムファイル、後者がXgpro用の
プログラムファイルになります。お使いのPICライタに合わせて、
適切なファイルを利用してください。
再度Enterを押すとウィンドウが閉じます。


※より詳細な手順等に関しては、以下のCPicSKのマニュアルを
参照してください。

https://bit.ly/cpicsk_manual_simple.pdf


■動作環境

64bit版および32bit版のWindows 10上で動作することを確認しています。

それ以外のバージョンのWindows上でも動作するかもしれませんが未確認です。


■ソースコード

各ツールのソースコードは以下から入手可能です。

* cpicsk_gen: https://github.com/nosuke192168920/cpicsk_gen
* kabuki_config_gen: https://github.com/nosuke192168920/kabuki_config_gen


■著作権表記

Copyright (c) 2024 nosuke <<sasugaanija@gmail.com>>


■ライセンス

各ツールのライセンスはMITライセンスに基づきます。

