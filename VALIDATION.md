# 検証記録

2026-10-19 / Robot Eyes v0.1

- PlatformIO `pio run`：SUCCESS。
- Espressif32 6.12.0 / Arduino-ESP32 2.0.17 / ESP32-S3。
- M5GFX 0.2.33、M5Unified 0.2.26、M5Chain 1.0.11（platformio.ini内のコミットで固定）。
- 静的RAM 25,488 bytes / 327,680 bytes。
- アプリFlash 557,545 bytes / 3,342,336 bytes。
- ホスト側ロジック検証：PASS。AddressSanitizerとUndefinedBehaviorSanitizerを使用。
- 表情の左右プレビューを目視確認：expressions.png。
- 実機接続・書き込み・LED通信・録音感度の確認は未実施。
- 音量ゲート方式であり、音声VAD・言語認識・話者識別は未実装。

開発パッケージ取得時に配布ミラーのチェックサム不一致が発生しましたが、別ミラーから
正しいチェックサムのパッケージを取得した後、ビルドが完了しました。
