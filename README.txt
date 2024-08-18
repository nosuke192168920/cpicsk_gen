■概要

kabuki_config_gen および cpicsk_gen は、CPicSKの
PIC12F509に書き込むプログラムを生成する専用ツールです。


■使い方

1. GitHubのMAMEのソースコードツリーより、kabuki.cpp を
ダウンロードして、kabuki_config.exe や cpicsk_gen.exe と
同じフォルダに置いてください。

https://github.com/mamedev/mame/blob/master/src/mame/capcom/kabuki.cpp


2. kabuki_code_gen.exe をダブルクリックするなどして
起動してください。

コマンドプロンプトが開き、KABUKI搭載ゲーム基板の一覧が
番号とともに表示されます。


3. CPicSKを装着する予定のゲーム基板の番号を半角数字で
入力してEnterを押してください。正しく動作している場合、
config.txt というファイルが生成されます。
再度Enterを押すとウィンドウが閉じます。


4. cpicsk_gen.exe をダブルクリックするなどして
起動してください。再びコマンドプロンプトが開き、
先程選択したゲームのタイトルが表示されるはずです。

このとき、cpicskprg.hex とcpicskprgx.hexの2つのファイルが
生成されます。前者が通常のPICライタ用のプログラムファイル、
後者がXgpro用のプログラムファイルになります。
お使いのPICライタに合わせて適切なファイルをご利用ください。
再度Enterを押すとウィンドウが閉じます。


※より詳細な手順等に関しては、以下のCPicSKのマニュアルを
参照してください。

https://bit.ly/cpicsk_manual


■動作環境

64bit版およびWindows 10上で動作することを確認しています。

32bit版のWindows 10や、Windows 10以外のバージョンの
Windows上でも動作するかもしれませんが未確認です。


■ソースコード

各ツールのソースコードは以下から入手可能です。

* cpicsk_gen: https://github.com/nosuke192168920/cpicsk_gen
* kabuki_config_gen: https://github.com/nosuke192168920/kabuki_config_gen


■著作権表記

Copyright (c) 2024 nosuke <<sasugaanija@gmail.com>>


■ライセンス

各ツールのライセンスはMITライセンスに基づきます。


■バージョン情報

* v1.1a (2024/8/19)
** cpicsk_gen にバージョン表示のための-vオプションを追加
** kabuki_config_gen のバージョン表示形式を変更
** 本ドキュメントのマニュアルのURLを修正

* v1.1 (2024/6/24)
** PICがオシレータのキャリブレーション値を使うようにプログラムの
テンプレートを修正
** 一部のタイトルのコンフィグレーションの末尾2Byteを修正

* v1.0 初版 (2024/3/20)
