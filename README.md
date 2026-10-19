# Robot Eyes v0.2

管理リポジトリ：https://github.com/h-vis/m5stack_robot_eye

段ボールのロボット仮装用。PC・Wi-Fiなしで動作するArduino C++プロジェクトです。

## 対象機材

**C126-ECHO（Atom EchoS3R／現名称Atom VoiceS3R）→ U207 → U218左目 → U218右目**。
通常のC126（画面付きAtomS3R）用ではありません。

- C126-ECHOのGrove端子：黄色GPIO2=TX、白色GPIO1=RX、赤5V、黒GND。
- 各Chain機器の入力・出力方向を合わせて接続してください。
- USB-Cから給電。電源を切ってから配線します。
- スピーカー再生は無効です。内蔵マイクだけを使用します。

## 実装した操作

|操作|動作|
|---|---|
|U207回転|水色→青→緑→黄→オレンジ→赤→ピンク→紫、逆回転で逆順|
|U207短押し|ふつう→にこにこ→おこる→かなしい→びっくり→ハート|
|U207長押し1.5秒|LED点灯・消灯（音声処理は継続）|
|音を検出|左右同期して明るさ変化。強い音ではふつう・びっくりのみ拡大|
|無音|約300msの発話間隔を許容し、明るさは約300msの時定数で元へ戻る|
|待機|3〜6.5秒間隔でまばたき|

初期値は水色・ふつう・点灯。設定はRAMのみで、再起動で初期値へ戻ります。
表情変更時には180msのまばたきが入ります。右目のおこる・かなしいは左右反転です。
U218の輝度上限は35%、通常のピクセル明度45%、音反応時は最大90%です。

## 初版の音声反応の範囲

起動後約1.2秒は静かにして環境音を測ります。直流成分を除いた20ms音声のRMSを計算し、
環境音に追従するしきい値と比較します。明るさは急に点滅させず平滑化します。
**これは音量による反応で、声と雑音を分類するVAD、言葉の認識、話者識別は未実装です。**
拍手・風・段ボールを触った音にも反応します。屋外でのVAD導入は次の段階です。
マイクを口元に近づけ、穴をテープで塞がないよう取り付けてください。

## 書き込み（VS Code＋PlatformIO）

1. PCにVS Codeと拡張機能「PlatformIO IDE」をインストール。
2. このフォルダー（platformio.iniのある階層）を開く。
3. C126-ECHOをデータ通信対応USBケーブルで接続。
4. PlatformIOのBuildを実行してからUploadを実行。
5. Serial Monitor（115200bps）を開く。

起動ログを見逃した場合は、モニターを開いたまま本体をリセットしてください。

コマンドの場合：

```sh
pio run
pio run -t upload
pio device monitor -b 115200
```

USBが認識されない場合は、リセットボタンを約2秒長押しし、内部の緑LEDが光ったら離して
ダウンロードモードにしてからUploadしてください。書き込み後に必要ならリセットします。
モニターを開くまで処理を止めないので、モバイルバッテリーだけでも起動します。

依存ライブラリは公式GitHubのコミットに固定しています。初回はネット接続が必要です。
PlatformIO設定はESP32-S3・8MB Flash・OPI PSRAM用です。

## ビルド済みファームウェアを書き込む場合

`firmware/robot-eyes-v0.2.bin` はブートローダー・パーティションを含む結合バイナリです。
**C126-ECHO専用、書き込み開始アドレスは0x0です。** 元のアプリとパーティションを上書きします。
調整が不要なら、PCにPythonを入れ、次のコマンドでコンパイルせず書き込めます。
`COM5` は実際のポート名に変えてください。コマンドはこのフォルダーで実行します。

```sh
python -m pip install esptool==4.9.0
python -m esptool --chip esp32s3 --port COM5 --baud 460800 write_flash 0x0 firmware/robot-eyes-v0.2.bin
```

書き込みモードへの入り方は上記と同じです。書き込み後は本体をリセットします。
設定や表情を変更した場合は、PlatformIOから再ビルド・Uploadしてください。

## 初回の確認

1. 机上で接続して電源ON。約1.2秒静かにする。
2. ログに `microphone=OK` と `Ready: encoder=1 left=2 right=3` が出ることを確認。
3. 両目が水色になり、回すと色、短押しで表情、長押しで点灯状態が変わることを確認。
4. 話すと明るさが変化し、大きな音でふつうの目が拡大することを確認。
5. 実装着状態で感度・LED方向を調整。

接続台数はU207が1台、U218が2台の計3台を要求します。通信を失ったら3秒ごとに再検出します。
UARTはメインループだけで使用し、録音は別タスクで行います。

## 調整する場所

`include/Config.h`：

- `LEFT_ROTATION` / `RIGHT_ROTATION`：0/1/2/3で0/90/180/270度。
- `SWAP_EYES`：左右を取り違えた場合true。
- `ENCODER_DIRECTION`：逆向きなら-1。
- `ENCODER_COUNTS_PER_COLOR`：ノブ1クリックで2色/4色進むなら2/4。
- `MIN_RMS`：小声に反応しなければ下げる（初期250）。
- `NOISE_RATIO`：環境音で反応しすぎれば上げる（初期2.8）。
- `BRIGHTNESS_LIMIT`：まず35%で使用。メーカー推奨に合わせ50%以下にする。

`include/Eyes.h`：色、6表情の8行ビットマップ、音量平滑化、ボタン判定。
`src/main.cpp`：機器接続、録音、操作、描画。

## 検証と未確認事項

ホスト検証：

```sh
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude test/test_logic.cpp -o /tmp/robot-eyes-test
ASAN_OPTIONS=detect_leaks=0 /tmp/robot-eyes-test
```

ボタンのチャタリング・長押し重複防止・時計周回、音反応の保持/減衰、48種類の色と表情、
左右反転、まばたきと消灯を検証します。ビルド結果はVALIDATION.md参照。
実機はこの環境に接続されていないため、UART応答・ノブ1クリックのカウント数・取付方向・
実際の音声感度・電源安定性は利用者の実機で確認が必要です。

## 公式資料

- https://docs.m5stack.com/en/core/Atom_VoiceS3R
- https://docs.m5stack.com/en/arduino/atom_echos3r/mic
- https://docs.m5stack.com/en/chain/Chain_Encoder
- https://docs.m5stack.com/en/chain/Chain_RGB
- https://github.com/m5stack/M5Chain
- https://github.com/m5stack/M5Unified

## アイドル時の視線移動
無音時は4〜8秒おきに左右どちらかへ1ドット移動し、約0.9秒後に正面へ戻ります。音に反応したときは正面へ戻ります。


音に反応している間は、選択中の表情にかかわらず丸い目になります。静かになると選択した表情へ戻ります。
