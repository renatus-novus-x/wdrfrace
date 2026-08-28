# Wire Drift Racers

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/wdrfrace/main/dist/wdrfrace.xdf&run=1">WebX68kでWire Drift Racersを起動</a></strong>
</p>

Wire Drift Racersは、Sharp X68000向けのベアメタル3Dワイヤーフレーム・
バトルレーシングゲームです。C++で実装し、Human68kランタイムを使わずXDFから
直接起動します。

## 特徴

- 5段階のCPUと戦う1 PLAYER、または1台で遊ぶオフライン2 PLAYERS
- RING、OVAL、PULSEの3種類のワイヤーフレームコース
- ブースト、アクティブゲート、ドリフトタックル、後続回復、スリップストリーム
- 固定20 Hzのゲーム進行と60 Hz表示タイミングのダブルバッファ描画
- タイトル、コース選択、操作説明、デモ、レース、リザルト、サウンドテスト
- 低負荷NDP STREAM BGMと事前計算STREAM SE
- キーボードおよび2台のゲームパッドに対応

## 操作

| 操作 | PLAYER 1 | PLAYER 2 | ゲームパッド |
|---|---|---|---|
| 加速／減速 | `W`／`S` | カーソル上下 | 上下 |
| ドリフト | `A`／`D` | カーソル左右 | 左右 |
| ブースト | `Q` | `N` | ボタン1 |
| ブレーキ | `E` | `M` | ボタン2 |

メニューでは上下で項目選択、左右で設定変更、`SPACE`またはボタン1で決定します。
対応画面では`ESC`またはボタン2で戻ります。テスト画面では`D`でFPSと座標軸の
デバッグ表示を切り替えられます。

## ビルド

WSL Ubuntu 24.04、[elf2x68k](https://github.com/yunkya2/elf2x68k)、GNU Make、
Python 3、`curl`が必要です。

```sh
cd src
make clean
make
```

既定値は公開用ビルドです。ローカルに`src/bgmpriv.h`が存在しても、購入した
NDP DemoSongsを自動では組み込みません。次のファイルを生成します。

- `src/human.sys`
- `dist/wdrfrace.xdf`

正規に入手したSTREAM曲ヘッダと`src/bgmpriv.h`を用意し、私用XDFを作る場合だけ
明示的に有効化し、公開版とは別名で出力してください。

```sh
cd src
make clean
make PRIVATE_BGM=1 XDF=../dist/wdrpriv.xdf
```

`bgmpriv.h`、私有曲の生成ヘッダ、`wdrpriv.xdf`、購入曲入りXDFは公開しないでください。

## サウンドの組み込み

[`src/ndp.h`](src/ndp.h)はX68000向けNDP単一ヘッダプレーヤーです。BGMは低負荷な
`NDP_PROFILE_STREAM`を使用し、60 Hz V-DISP割り込みから更新します。ゲーム独自SEは
[`src/wdr_se.mml`](src/wdr_se.mml)で定義し、ビルド時に`NDSS`バンクへ変換して、
BGMストリームと重ならない専用チャンネルで再生します。

公開ビルドにはオリジナルSEだけを含み、購入BGMは含みません。独立したSTREAM
BGM＋STREAM SEサンプルと生成ツールは
[ndp-x68k](https://github.com/renatus-novus-x/ndp-x68k)を参照してください。

## ドキュメント

- [日本語取扱説明書](doc/manual-ja.md)
- [英語取扱説明書](doc/manual-en.md)
- [ゲーム仕様書](doc/spec.md)
- [開発予定](doc/roadmap.md)
- [開発進捗](doc/progress.md)

## ライセンス

ソースコードはMIT Licenseです。ゲーム独自SEのMMLはCC0-1.0です。NDP DemoSongs
などの購入曲は、このリポジトリおよび公開ビルドには含みません。詳細は
[LICENSE_NOTES.md](LICENSE_NOTES.md)と
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)を参照してください。
