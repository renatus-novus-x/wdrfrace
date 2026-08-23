# Wire Drift Racers

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/wdrfrace/main/dist/wdrfrace.xdf&run=1">WebX68kでWire Drift Racersを起動</a></strong>
</p>

Wire Drift Racersは、Human68kのランタイムAPIを使用しないSharp X68000向けベアメタル3Dワイヤフレームレーシングゲームプロジェクトです。C++で実装しています。

## 現在のプロトタイプ

- 4辺で構成したワイヤフレーム車体を回転表示
- 前フレームを黒線で消してから、次フレームを白線で描画
- `_iocs_line()`を使用し、全画面を再描画しない線描画
- 256段階の三角関数参照テーブルを使用
- 300フレームの平均FPSを計測して画面へ表示
- テスト環境で現在約15.50 FPSを確認

現在のXDFは描画と性能を確認するためのプロトタイプです。レース部分と2人プレイ操作はまだ実装していません。

## ゲーム完成時の予定

- 1台のX68000を使用するオフライン・ローカル2人対戦
- 3Dワイヤフレームのリングコースを見渡す固定斜め俯瞰カメラ
- 加速、減速、ドリフト、レーン移動、回数制限付きブースト
- 3周先取、または90秒タイムトライアル
- 車体、コース壁、ランプ、ゲート、背景グリッドをワイヤフレームのみで描画

## 操作

- `ESC`: プロトタイプを終了

## ビルド

WSL Ubuntu 24.04、導入済みの`elf2x68k`ツールチェーン、`python3`、`curl`が必要です。

```sh
cd src
make
```

生成物:

- `src/human.sys`
- `dist/wdrfrace.xdf`

生成したXDFを検査する場合:

```sh
cd src
make check-xdf
```

## ドキュメント

- [ゲーム仕様書](doc/spec.md)
- [ゲーム仕様スライド](doc/spec.pptx)
- [開発進捗](doc/progress.md)
