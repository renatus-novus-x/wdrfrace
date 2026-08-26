# Progress Log

## 2026-08-23

- 初期仕様確定: 2P オフライン対戦前提の 3D ワイヤフレームレーサーとして `Wire Drift Racers` を採用。
- 仕様書を匿名化済みで `doc/spec.md` に反映（ゲームルール、画面イメージ、開発優先順位、タイミング条件を統合）。
- `Makefile` を `human.sys` 起動名＋`-specs=x68knodos.specs` 構成に更新。
- `wdrfrace.xdf` を再生成し、`dist/wdrfrace.xdf` と `src/human.sys` を出力。
- `check-xdf` 実行で `.xdf` 生成状態を確認。
- 次アクション: 描画負荷を分析・削減してフレームレートを改善した後、2P 入力と車両移動・衝突を追加する。

### 追加実装（同日）
- 差分描画の線色を白に変更し、前フレームを黒線で消してから白線を再描画する方式へ調整。
  - これにより全画面クリアを避け、高速化した描画ループを維持。
- doc/spec.md / doc/spec.pptx を最終仕様セット（画面イメージ・コア仕様・画面構成・データ設計・実装方針）で更新完了。

### FPS 計測とビルド（同日）

- 300 フレームの経過時間から平均 FPS を算出し、画面上部へ表示する計測機能を追加。
- 現行プロトタイプの実測値は約 4.35 FPS。60 FPS の達成には描画・3D 計算の大幅な高速化が必要。
- WSL の Ubuntu 24.04 に構築済みの elf2x68k 環境を使用して再ビルド。
- `-specs=x68knodos.specs` で `human.sys` を生成し、Human68k を含まない `dist/wdrfrace.xdf` の作成に成功。

### リポジトリ整理（同日）

- `.gitignore` を追加し、`src` 配下のオブジェクト、`human.sys`、ビルド作業ディレクトリ、OS・エディタの一時ファイルを除外。
- `doc/.tmp-spec-pptx` など、`doc` 直下で `.tmp` から始まる一時ディレクトリ・ファイルを除外。
- 配布用成果物をコミットできるよう、`dist` 配下は除外対象にしていない。
- 現段階を初回プロトタイプのコミット候補とする。

### 描画高速化 第1段階（同日）

- ソフトウェア線描画内のピクセル単位 `_iocs_fill()` を、1本単位の `_iocs_line()` 呼び出しへ変更。
- 頂点ごとに重複していた `sinf()` / `cosf()` を頂点ループの外へ移動し、フレーム内で回転係数を共有。
- FPS HUD の再描画を毎フレームから、初回および300フレームの計測完了時だけに削減。
- 車体モデルの重複エッジ4本を削除し、描画対象を24本から20本へ削減。
- Ubuntu 24.04上のelf2x68k環境で再ビルドし、`dist/wdrfrace.xdf` を更新。
- 実機またはエミュレーター上の300フレーム平均で約15.50 FPSを確認。変更前の約4.35 FPSから約3.6倍に改善。
- 高速化第1段階の動作確認が完了したため、ここまでをコミット候補とする。
- 次の高速化候補は、3D計算の固定小数点化とGVRAM直接線描画。まず30 FPS以上を中間目標とする。

### 描画高速化 第2段階（同日）

- 車体モデルを12頂点・20辺から、横長の台形を構成する4頂点・4辺へ簡略化。
- 1フレームの車体描画を黒20本＋白20本から、黒4本＋白4本へ削減。
- 256段階のサイン参照テーブルを追加し、コサインは4分の1周期の位相差で参照。
- テーブルは起動時に漸化式で生成し、フレームループ内の `sinf()` / `cosf()` 呼び出しを廃止。

### README整備（同日）

- `pong` と同じ言語切り替え構成で、英語版 `README.md` と日本語版 `README.ja.md` を追加。
- 現在のプロトタイプ、計画中のゲーム仕様、操作、ビルド方法、WebX68k起動リンク、各種ドキュメントへのリンクを記載。

### 回転方式修正（同日）

- 車体に適用していたY軸回転とX軸回転の組み合わせを廃止。
- 投影面に平行なZ軸回転だけを適用し、4辺の台形が回転中に極端に細くならない方式へ変更。
- 現行投影座標は右が+X、上が+Y、カメラ前方が+Zであり、OpenGLのカメラ前方-Zとは異なることを確認。
- Ubuntu 24.04上のelf2x68k環境で再ビルドし、`dist/wdrfrace.xdf`を更新。

### カメラ周回表示（同日）

- 車体自身の回転を廃止し、車体をワールド原点へ固定。
- カメラが原点の周囲を周回するよう、カメラ角度の逆回転をビュー変換へ追加。
- 車体とRGBのXYZデバッグ座標軸へ同じビュー変換を適用し、画面上で両方が一体となって回転する表示へ変更。
- Ubuntu 24.04上のelf2x68k環境でカメラ周回版をビルドし、`src/human.sys`と`dist/wdrfrace.xdf`の生成に成功。
- OpenGL準拠座標系とカメラ周回表示までをコミット候補とする。

### 座標軸の差分消去修正（同日）

- カメラ周回後の新しいXYZ軸だけを描画し、旧位置の軸を消していなかった問題を修正。
- XYZ軸についても前フレームの投影座標と可視状態を保持。
- 旧車体と旧座標軸を黒線で消去してから、新座標軸と新車体を描画する順序へ変更。
- Ubuntu 24.04上のelf2x68k環境でリビルドし、`dist/wdrfrace.xdf`を更新。

### 固定20 Hz物理・20 FPS描画へ統一

- 物理更新と描画をともに5 cs間隔へ統一し、100 Hz時計で端数の出ない固定20 Hz構成へ変更。
- 物理用・描画用の複数rate accumulatorと描画補間を削除し、単一のcentisecond accumulatorへ簡略化。
- 5 cs到達ごとに物理を1回更新し、その後に描画を1回実行。
- 遅延時は固定物理更新を必要回数実行するが、描画は最後の状態を1回だけ行い、最大20 FPSに制限。
- ポーズ中は時計と描画周期を継続し、物理更新だけを停止。
- カメラ回転量を1物理ステップあたり0.045 radとし、回転速度を約0.9 rad/sに維持。

### Cameraクラス導入

- `camera.h/.cpp`を追加し、OpenGL準拠右手座標系の`Camera::look_at(eye, target, up)`を実装。
- `look_at()`でright、up、backward基底と平行移動を含む3×4ビュー行列を生成し、`world_to_view()`で頂点を変換する構造へ変更。
- 既存のカメラ位置`(12 sinθ, 5, 12 cosθ)`、原点注視、+Y up、焦点距離320の透視投影を維持。
- GameModeTestから直接記述していたビュー変換式を削除し、車体とXYZデバッグ軸の双方をCameraクラス経由で変換。
- ビュー行列は固定20 Hzの物理更新時だけ再計算し、描画時はキャッシュ済み行列を利用。
- 追加された`camera.cpp`はMakefileの`*.cpp`自動検出により設定変更なしでビルド対象となる。
- Ubuntu 24.04上のelf2x68k環境でリビルドし、`dist/wdrfrace.xdf`を更新。

## 2026-08-24

### 固定物理更新と描画レート分離

- Applicationへ1/100秒単位の経過時間計測、物理accumulator、描画レートaccumulatorを追加。
- GameModeの`update()`を`update(uint16_t fixed_dt_cs)`へ変更し、5 cs刻みの20 Hz固定物理更新を実装。
- GameModeの`render()`を`render(uint16_t alpha_q8)`へ変更し、前回と現在の物理状態を8 bit係数で補間。
- 描画は100 csあたり15回のrate accumulatorで制限し、6 csと7 cs間隔を組み合わせて最大平均15 FPSとした。
- FPS HUDの計測対象を物理更新回数ではなく実際の描画回数へ変更。
- 1フレームの経過時間を最大10 csへ制限し、長時間停止後の過剰な追いつき更新を防止。
- ポーズ中も時計を更新し、解除時に停止時間分の物理ジャンプが発生しない構造を維持。
- Ubuntu 24.04上のelf2x68k環境でリビルドし、`dist/wdrfrace.xdf`を更新。

### カメラ回転軸の簡略化（同日）

- 複合的な固定斜め視点とZ軸周回を廃止。
- カメラの周回をワールドY軸方向だけに限定。
- ビュー変換をY軸の逆回転とカメラ前方Z=-12への移動だけに簡略化。

### Y軸周回カメラの明確化（同日）

- カメラ位置を`(R sinθ, H, R cosθ)`とし、Y軸から半径R=12、高さH=5を保って周回する方式へ変更。
- カメラは周回中も常にワールド原点を見る右手座標系のビュー変換を使用。
- カメラへ高さを持たせ、視線と+Z軸が完全に重なって青線が消える状態を回避。
- RGB座標軸を車体の後ではなく手前に描き、デバッグ表示の視認性を改善。
- Ubuntu 24.04上のelf2x68k環境でリビルドし、`src/human.sys`と`dist/wdrfrace.xdf`の生成に成功。

### ダーティ矩形消去の比較実装（同日）

- 前フレームの車体とXYZ軸の投影点から、1ピクセル余白付きの最小スクリーンAABBを計算。
- 従来の黒線7本による消去を、AABBに対する1回の`_iocs_fill()`へ置き換える実験実装を追加。
- `make DIRTY_RECT=1`をAABB消去、`make DIRTY_RECT=0`を従来の黒線消去として`#ifdef`で切り替え可能にした。
- 設定変更時は`make clean`後に再ビルドし、300フレーム平均FPSで比較する。
- AABB方式はIOCS呼び出し回数を削減する一方、矩形内部まで書き込むため、形状と角度によってメモリアクセス量が増える点を評価対象とする。
- Ubuntu 24.04上のelf2x68k環境で`DIRTY_RECT=1`をリビルドし、`dist/wdrfrace.xdf`を更新。
- `DIRTY_RECT=1`版で約30.30 FPSを確認し、従来方式との差は現時点では判別しにくい状態。
- 既存の`dist/wdrfrace.xdf`をAABB消去版として保持したまま、`DIRTY_RECT=0`の黒線消去版を`dist/wdrline.xdf`として追加生成。
- `wdrfrace.xdf`と`wdrline.xdf`を同一条件で実行し、300フレーム平均FPSを直接比較可能にした。

### Application / GameModeクラスへのリファクタリング（同日）

- `Application`クラスを追加し、`application_initialize()`、`application_update()`、`application_finalize()`へ画面初期化、垂直同期、モード更新、終了復元を集約。
- 仮想関数`initialize()`、`update()`、`finalize()`を持つ`GameMode`基底クラスを追加。
- 現在のカメラ周回・車体・FPS計測処理を`GameModeTest`派生クラスへ移動。
- `Application`が現在の`GameModeId`と`GameMode`ポインタを保持し、今後のタイトル・ゲーム・リザルトモード追加に対応できる構造へ変更。
- XYZデバッグ軸のモデル、現在座標、前フレーム座標、可視状態、投影・描画処理を`GameModeTest`のメンバへ集約。
- `main()`をApplicationの初期化、更新ループ、終了処理だけに短縮。
- AABBと黒線消去の性能差を確認しやすいよう、車体を画面中央の大部分を占めるサイズへ拡大。
- `DIRTY_RECT=1`を`dist/wdrfrace.xdf`、`DIRTY_RECT=0`を`dist/wdrline.xdf`として再生成。

### ダーティ矩形方式の廃止（同日）

- 画面内で車体を大きく表示した条件で、AABB矩形消去版は約13.29 FPS、黒線7本消去版は約30.15 FPSを確認。
- AABB内部全体への書き込み負荷が、IOCS呼び出し回数を減らす効果を上回ると判断。
- `USE_DIRTY_RECT_CLEAR`による条件分岐とAABB計算処理を削除し、黒線7本による消去へ一本化。
- Makefileから`DIRTY_RECT`切り替え設定を削除。

### Application更新・描画分離とソース分割（同日）

- Application APIを`initialize()`、`update()`、`render()`、`finalize()`へ整理。
- GameModeの仮想関数を`update()`と`render()`へ分離し、状態更新を止めても描画を継続できる構造へ変更。
- Applicationへポーズ状態と`set_paused()`を追加し、ポーズ中はGameModeの`update()`を呼ばず`render()`だけを継続可能にした。
- `Application`を`app.h/.cpp`、`GameMode`を`gmode.h/.cpp`、`GameModeTest`を`gmtest.h/.cpp`へ分割。
- `main.cpp`はApplicationの初期化、更新・描画ループ、終了だけを行う構成へ簡略化。
- elf2x68k付属ヘッダーと同じ`#ifndef`/`#define`形式のinclude guardを各ヘッダーへ追加。
- Makefileで`$(wildcard *.cpp)`から全C++ソースを自動収集し、`-MMD -MP`によるヘッダー依存関係の自動追跡を追加。

### デバッグ表示切り替え（同日）

- X68000のDキー（スキャンコード`0x20`）で、FPS HUDとRGBのXYZ座標軸をまとめて表示・非表示できるトグルを追加。
- 小文字`d`と大文字`D`は同じ物理キーとして扱い、押し続けても連続反転しない押下エッジ判定を実装。
- 非表示へ切り替えたフレームで旧XYZ軸とFPS領域を黒く消去し、以降の軸投影と描画を省略。
- 再表示時は現在のXYZ軸と最新FPS値を即時描画。
- Ubuntu 24.04上のelf2x68k環境でリビルドし、`dist/wdrfrace.xdf`を更新。

### OpenGL準拠座標系とデバッグ軸（同日）

- 座標系を右が+X、上が+Y、カメラ前方が-ZのOpenGL準拠右手座標系へ変更。
- ワールド座標を固定斜め視点へ変換し、原点をカメラ前方のZ=-12へ配置。
- 原点から長さ1の+X軸を赤、+Y軸を緑、+Z軸を青で表示するデバッグ描画を追加。
- 車体はワールド原点を中心としてZ軸回転し、固定座標軸と同時に表示。
- Ubuntu 24.04上のelf2x68k環境で再ビルドし、`dist/wdrfrace.xdf`を更新。
## 最小3D数学クラスの導入

- `math3d.h` / `math3d.cpp` に、GLM風の最小構成として `Vec3f` と `Mat34f` を追加した。
- `Vec3f` は減算、符号反転、内積、外積、長さ、正規化を提供する。
- `Mat34f` は単位行列、ビュー行列設定、点のアフィン変換を提供する。
- `Camera` が独自のベクトル関数や `float[3][4]` を直接扱わず、数学クラスを利用する構成へ変更した。
- X68000での負荷とコード量を抑えるため、動的メモリ、テンプレート、未使用の4x4演算は導入していない。
- 既存のY軸周回カメラは `Camera::look_at()` で同じ挙動になるよう移行した。
- Makefileの `*.cpp` 自動検出により、追加した `math3d.cpp` もMakefileを変更せずビルド対象になった。
- WSL Ubuntu 24.04上のelf2x68k環境でクリーンビルドに成功した。
- `-specs=x68knodos.specs` で `human.sys` を生成し、`dist/wdrfrace.xdf` を再生成した。

## 2026-08-25

### Scanner Garage title presentation update

- Replaced the single moving scan line with animated wireframe car highlights and runway guide lights.
- Added eight-step brightness gradients for the red and blue cars, using palette color replacement instead of alpha blending.
- Made a white highlight travel along each car's edges in opposite directions to create continuous motion with few changed pixels.
- Added sequential runway lights between the two cars to reinforce depth without a large moving line.
- Kept four fixed cinematic camera cuts to contrast the calm title presentation with the faster DEMO REPLAY sequence.
- Moved camera projection and line clipping to `mkhero.py`; runtime rendering reads precomputed `HeroShot` and `HeroFrame` data from `herodat.h`.
- Avoided black-line erasure during ordinary title frames. Only changed car edges and guide lights are repainted, reducing flicker and write volume.
- Updated the generated-header rule for `herodat.h` and added `/src/*.h.tmp` to `.gitignore` for interrupted atomic-generation artifacts.
- Rebuilt successfully with the elf2x68k environment on WSL Ubuntu 24.04 and regenerated `dist/wdrfrace.xdf`.

### Python build cache exclusion

- Added `__pycache__/` and `*.py[cod]` to `.gitignore`.
- Python bytecode caches are generated from the build-time source scripts and are not required to build or run the XDF image.

### 60 Hz double buffering and transition fixes

- Changed the graphics mode from 512x512 65536-color single-page mode to 512x512 256-color two-page mode while retaining the custom 60 Hz CRTC timing.
- Added `APAGE` back-buffer drawing and VBlank-synchronized `VPAGE` display switching.
- Added page-specific previous-frame state for title, demo, race, car, and test-mode rendering so each back buffer can be updated incrementally.
- Added a fixed 21-color palette that preserves the red and blue car gradients, track colors, HUD colors, and debug-axis colors.
- Restricted IOCS drawing to the 512x480 visible area with `WINDOW(0,0,511,479)`.
- Replaced title camera-cut erasure with one rectangular clear of the 3D presentation area.
- Changed the race intro to rebuild the current precomputed frame on the hidden page while keeping normal gameplay on incremental line erasure and repair.
- Analyzed every frame of the supplied `5.gif` and `6.gif` captures to isolate two double-buffer boundary defects.
- Fixed demo camera cuts where one page advanced by two frames and skipped the frame carrying `DEMO_FLAG_CUT`; a shot-number change now also forces replay-area reconstruction.
- Fixed a race-intro path that briefly entered normal gameplay rendering when a page already contained the current intro frame.
- Rebuilt successfully with the elf2x68k environment on WSL Ubuntu 24.04 and regenerated `dist/wdrfrace.xdf`.

## 2026-08-25: Race intro line erasure optimization

- Replaced the 512 x 480 full-screen clear during continuous race-intro camera animation with page-specific black-line erasure.
- Each hidden page now erases its previous car and track lines, then draws the current track and cars.
- Full-screen clearing remains only for the initial setup of each page after entering race mode.
- Confirmed that no per-frame or continuous-animation path performs a full-screen clear.
- Partial rectangular clears remain at title camera cuts and demo replay camera cuts; these do not clear the full screen.
- On-device observation showed a dramatic performance improvement after this change.

## 2026-08-25: Technical Monoline and telemetry typography

- Added the reusable `vtext.cpp` / `vtext.h` single-stroke vector font renderer based on IOCS line drawing.
- Replaced the main title logo with a slanted Technical Monoline `WIRE DRIFT RACERS` design.
- Added compact telemetry labels and horizontal reference lines to the title, demo replay, and how-to-play screens.
- Converted `DEMO REPLAY`, `HOW TO PLAY`, player labels, and the lap objective to the vector font.
- Kept compact supporting text on GVRAM instead of using IOCS `B_PUTMES`, because text VRAM is managed separately from the two GVRAM display pages.
- Added native-scale 5 x 7 telemetry text with configurable character tracking for prompts and control instructions.
- Updated `PRESS SPACE`, `PRESS ANY KEY`, and the how-to-play instruction text to use native-size tracked lettering instead of enlarged bitmap glyphs.
- Fixed the lower edge of `RACERS` being erased by the garage partial clear: the logo now ends at y=120 before the 3D clear region begins at y=124.
- Reduced both title-line scales and increased the gap between `WIRE DRIFT` and `RACERS` from 2 pixels to 12 pixels without overlapping the 3D region.
- Successfully rebuilt `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-25: Independently generated telemetry bitmap font

- Removed the previous hand-entered 5 x 7 letter and digit bitmap tables from `screen.cpp`.
- Added `mktele.py`, which defines A-Z and 0-9 as project-owned monoline strokes and rasterizes them to compact 5 x 7 glyphs with an integer line algorithm.
- Added a Makefile dependency that generates `telefont.h` atomically before compiling `screen.o`.
- Kept the compact font deterministic across X68000 hardware, ROM versions, and emulators so the telemetry layout and Technical Monoline identity remain consistent.
- Documented the glyph construction and provenance in `doc/font-origin.md`.
- Reserved runtime IOCS font retrieval with `_iocs_fntget()` as a possible future path for system messages or Japanese text rather than the primary game UI.
- Successfully regenerated `telefont.h`, rebuilt `human.sys` with `x68knodos.specs`, and created `dist/wdrfrace.xdf` using the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-25: Race completion, CPU opponent, and gamepad support

- Added one-player and two-player selection to the title screen, selectable with W/S, cursor keys, or either gamepad.
- Added a lightweight CPU driver for player 2 in one-player mode, using the same `CarInput` and vehicle physics as a human player.
- Added live three-lap counters for player 1 and player 2 or CPU to the race HUD.
- Added finish detection after both cars are updated in the same fixed 20 Hz physics step.
- Added player 1 win, player 2 or CPU win, and same-frame draw outcomes.
- Added `GAME_MODE_RESULT` and a dedicated `GameModeResult` class for displaying the race outcome and final lap counts.
- Restyled the result screen to match HOW TO PLAY, using single-stroke vector headings and compact Technical Monoline supporting text.
- Added IOCS `_iocs_joyget()` support with gamepad port 1 assigned to player 1 and port 2 assigned to player 2.
- Mapped gamepad directions to acceleration, deceleration, and lateral movement; button 1 to boost; and button 2 to brake.
- Added gamepad input to title selection, confirmation, HOW TO PLAY, demo exit, and result input handling.
- Changed result timing so held race controls must first be released before the result timer starts.
- The result remains visible for at least one second and returns automatically after 100 fixed updates, approximately five seconds at 20 Hz; a new input can skip it after the minimum display period.

## 2026-08-25: Three-dimensional race and demo car models

- Replaced the flat four-vertex, four-edge race car with a low-profile eight-vertex, twelve-edge wireframe model.
- Added separate body and roof outlines connected by four pillars, matching the three-dimensional construction used by the title presentation.
- Updated normal gameplay to project the eight car vertices at runtime while retaining black-line erasure, track repair, and two-page double buffering.
- Updated `mktitle.py` so all DEMO car vertices remain projected at build time and runtime replay only reads generated screen coordinates.
- Updated `mkintro.py` so all race-intro car vertices also remain projected at build time.
- Expanded the generated DEMO and intro data structures with explicit car vertex and edge counts.
- Kept the track geometry, camera calculations, and physics update rate unchanged so the visual-cost difference can be evaluated independently.
- Successfully regenerated `demodat.h` and `introdat.h`, rebuilt `human.sys`, and created `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-25: Active boost gates and HUD gauges

- Added three active boost gates at fixed positions around the race track.
- Each gate activates one of the inner, center, or outer lane regions and rewards the first correctly aligned car to cross it.
- Added a 300-point boost refill, an approximately three-second cooldown, and lane rotation when a gate reactivates.
- Added same-step claim resolution without a fixed player bias; an exact tie splits the reward between both cars.
- Updated the one-player CPU to steer toward the nearest active gate.
- Added page-aware gate erasure and redraw so gate state changes remain correct with two-page double buffering.
- Added eight-segment boost gauges below the P1 and P2 or CPU lap counters.
- Quantized the displayed boost value and redraw only segments whose state changed on each graphics page.
- Kept the gauges above the 3D viewport so car erasure and track repair cannot damage the HUD.

## 2026-08-25: Stronger boost response and visual feedback

- Separated normal and boosted speed limits so boost changes the actual race speed instead of only consuming the gauge near the normal limit.
- Set the normal maximum speed to 320 and the boosted maximum speed to 512.
- Increased boost acceleration to 32 per fixed physics update.
- Added a gradual 12-point-per-update return from boosted speed to the normal speed range after boost is released.
- Added high-brightness red and blue car colors while boost is active without adding any wireframe edges.
- Updated the CPU boost threshold so the CPU can use the expanded boosted speed range.
- Preserved the existing boost gauge, active-gate refill, fixed 20 Hz physics, and incremental two-page rendering.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Title-menu sound test and manual synchronization

- Promoted SOUND TEST from a hidden `S`-key shortcut to the third selectable title-menu item.
- Changed title up/down navigation to cycle through `1 PLAYER`, `2 PLAYERS`, and `SOUND TEST`.
- Restored `S` as the normal title-menu down input alongside cursor and gamepad navigation.
- Repositioned all three title choices and changed the prompt to `SPACE SELECT` without overlapping the 3D title viewport.
- Kept CPU LEVEL adjustment active only while `1 PLAYER` is selected.
- Documented SOUND TEST discovery and controls in the Japanese and English Markdown, PowerPoint, and PDF manuals.
- Updated the manuals with real-time countdown cues, final-lap warning audio, and winner-specific goal fanfares.

## 2026-08-26: Countdown, final-lap, and goal sound effects

- Added a reusable `GameSoundId` event path from game modes through `Application` to the non-blocking YM2151/OPM sound driver.
- Added short electronic countdown beeps when `3`, `2`, and `1` first appear.
- Added a brighter rising START sound at the exact fixed update where race controls become active.
- Added a two-tone final-lap warning when either car enters lap three for the first time.
- Added distinct goal fanfares for player 1, player 2 or CPU, and a simultaneous-finish draw.
- Keeps sound register access outside `GameModeRace`, allowing future race effects to reuse the same event interface.
- Emits each event once at its state transition rather than once per rendered frame.
- Preserved the fixed 20 Hz simulation, VBlank-stepped mode preparation, and non-blocking page-flip pipeline.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Course variations and interactive course selection

- Added three selectable course geometries: the balanced circular `RING`, wide `OVAL`, and three-apex `PULSE`.
- Applied the selected geometry consistently to race physics, track projection, the race-introduction sequence, and HUD course identification.
- Added `GAME_MODE_COURSE_SELECT` between title and HOW TO PLAY, with keyboard and gamepad left/right selection, confirmation, and return-to-title input.
- Added an animated TRACK CONFIG screen with rotating wireframe previews and course descriptions.
- Precomputed all three previews across 40 rotation frames at build time, leaving no runtime three-dimensional projection cost on the selection screen.
- Retained page-aware black-line erasure so animation works with the two-page double buffer without full-screen clearing.
- Completed the Technical Monoline vector font with all uppercase `A-Z` and numeric `0-9` glyphs after missing glyphs caused gaps in course names and SELECT COURSE.
- Raised the precomputed course preview center to `Y=168`, separating every course and rotation frame from the title, course-name, description, and control regions.
- Updated the Japanese and English user manuals in Markdown, PowerPoint, and PDF form with the new mode flow, controls, course descriptions, and course-specific strategy.

## 2026-08-25: Fair-start countdown

- Added a READY, 3, 2, 1, GO countdown after the precomputed race-intro camera sequence.
- Stops vehicle physics until GO so both players begin from the same simulation frame.
- Ignores acceleration, steering, braking, and boost effects during the countdown while continuing to scan input state.
- Requires each human player to release the boost button once before boost can be used, preventing a held pre-start button from producing an immediate boosted launch.
- Allows one-player CPU boost normally after GO without requiring a synthetic release.
- Displays each countdown stage in the reserved area above the 3D viewport.
- Tracks the displayed countdown stage independently for both graphics pages and erases only the previous label during transitions.
- Adds no full-screen clears and does not change the fixed 20 Hz physics or rendering schedule.

## 2026-08-25: Drift tackle interaction

- Added lightweight car-to-car contact detection using circular angle distance and lane-offset distance instead of three-dimensional mesh collision.
- A car drifting toward the opponent becomes the tackle attacker when both cars enter the contact range.
- A successful tackle pushes the defender 18 offset units and reduces its speed by 25 percent.
- The attacker receives a four-unit recoil and a 10-percent speed reduction, preventing risk-free attacks.
- Simultaneous opposing drifts knock both cars apart and reduce both speeds by 25 percent.
- Ordinary non-drift contact gives both cars a small separation push and an 8-percent speed reduction.
- Added extra speed loss when knockback pushes a car against the inner or outer lane limit.
- Added a six-update contact cooldown to prevent repeated impacts while the cars remain overlapped.
- Resolves tackles before active-gate claims so a player can push an opponent out of the valid gate lane.
- Added no wireframe edges, projections, or continuous HUD drawing, keeping the rendering workload unchanged.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-25: Distance-linked catch-up boost recovery

- Added total race-progress comparison using `lap * 65536 + angle` for both cars.
- Identifies the trailing car after each fixed physics update without changing the leader's vehicle parameters.
- Applies no assistance when the gap is less than one sixteenth of a lap.
- Recovers 4 boost points per update beyond one sixteenth of a lap, 8 beyond one eighth, and 12 beyond one quarter.
- Uses the existing capped `Car::add_boost()` path, so recovery never exceeds the 1000-point boost capacity.
- Applies the same recovery rules to human and CPU-controlled cars.
- Leaves maximum speed, acceleration, track geometry, and rendering unchanged; the existing boost gauge displays recovery automatically.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-25: Five-level CPU difficulty and humanized boost control

- Added CPU difficulty levels 1 through 5 for one-player races, with level 3 selected by default.
- Added CPU-level selection to the title screen using keyboard left/right, A/D, or either gamepad's left/right directions.
- Passes the selected level through `Application` to both HOW TO PLAY and `GameModeRace`.
- Displays the selected CPU level on the HOW TO PLAY screen.
- Replaced the CPU's continuous near-optimal boost input with level-dependent boost bursts and cooldown periods.
- Stops an active CPU boost burst when the CPU leads by at least one sixteenth of a lap.
- Adjusts gate-target decision intervals and deterministic targeting error by difficulty level, making lower levels slower to react and less precise.
- Keeps all CPU levels on the same vehicle physics, boost capacity, speed limits, and catch-up rules as the player.
- Fixed CPU-level digits accumulating on the title screen by erasing the previous page-specific menu strings in black before drawing the new level.
- Uses exact text erasure instead of a rectangular clear, preserving the incremental title renderer.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Proximity-based slipstream boost recovery

- Added slipstream detection for the trailing car within the near-gap region below one sixteenth of a lap.
- Requires the trailing car to remain outside tackle-contact distance and within 24 lane-offset units of the leader.
- Activates after 10 consecutive fixed updates, approximately half a second at the 20 Hz physics rate.
- Recovers 8 boost points per fixed update while active without changing speed, acceleration, or maximum-speed values.
- Resets immediately when the car leaves the distance window, changes away from the leader's lane, enters contact range, or becomes the leader.
- Keeps the existing distance-linked catch-up recovery for larger gaps, so the two recovery systems do not stack.
- Reuses the high-brightness car colors as a zero-edge visual indicator while slipstream recovery is active.
- Applies the same rule to human and CPU cars in one-player and two-player races.
- Updated the Japanese and English manuals with slipstream rules and strategy guidance.

## 2026-08-26: Accessible slipstream activation and feedback

- Investigated reports that slipstream recovery could not be activated reliably during normal play.
- Identified an overly narrow combination of distance, lateral-offset, and duration requirements rather than an unreachable arithmetic branch.
- Expanded the valid trailing distance from one sixteenth to one eighth of a lap while retaining the minimum separation outside tackle-contact range.
- Requires both cars to occupy the same logical lane and permits up to 48 offset units within that lane.
- Reduced lock-on time from 10 to 6 fixed updates, approximately 0.3 seconds at 20 Hz.
- Increased recovery from 8 to 12 boost points per update so the gauge rises even while boost consumes 8 points.
- Prevents distance-linked catch-up recovery from stacking with active slipstream recovery.
- Added a steady `DRAFT` HUD state during lock-on, followed by the existing blinking `SLIP`, high-brightness car, and blinking rear-edge feedback.
- Updated the Japanese and English manuals with the revised activation window, HUD states, recovery rate, and strategy.
- Synchronized the revised instructions across the Markdown, PowerPoint, and PDF manual editions while preserving the existing slide design.
- Verified all 17 slides in both PowerPoint editions have no content overflow and visually checked the affected PowerPoint and PDF pages.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Per-shot title wireframe framing

- Added build-time two-dimensional BoundingBox fitting to all four title-screen camera shots.
- Includes the floor grid, animated light lines, and both three-dimensional car models when calculating each shot's bounds.
- Fits every shot uniformly into the reserved `x=28..484`, `y=140..374` viewport between the title graphics and lower menu.
- Uses one shared scale and translation per shot, preserving the original camera composition without frame-to-frame size changes.
- Emits source bounds, fitted bounds, and scale values as comments in the generated `herodat.h` for future tuning.
- Adds no runtime projection, geometry, or drawing cost because all adjusted screen coordinates remain precomputed at build time.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Real-time countdown and data-driven goal fanfare

- Investigated uneven audible spacing between the `3`, `2`, and `1` countdown cues.
- Identified that countdown timing and sound dispatch were tied to fixed 20 Hz physics updates, which can execute more than once after a delayed render and therefore produce uneven wall-clock spacing.
- Added `GameMode::advance_time()` so presentation timing can use measured centiseconds independently of fixed-step vehicle physics.
- Changed the race countdown to advance from real elapsed time immediately after VBlank handling.
- Triggers `3`, `2`, and `1` at exact 0.75-second boundaries and starts the START cue at the matching control-release boundary.
- Kept all vehicle input and physics calculations on the existing fixed 20 Hz update path.
- Replaced hard-coded two-note SE progression with a compact data-driven sequencer using pairs of OPM key codes and 20 Hz note durations.
- Migrated confirmation, cancellation, selection, countdown, START, and final-lap sounds to the shared sequence format.
- Replaced the single goal tone with a four-note fanfare lasting approximately 0.5 seconds.
- Added separate rising fanfare registers for player 1 and player 2 or CPU, plus a neutral alternating phrase for a draw.
- Keeps goal fanfares free of the previous noise transient so the melody starts cleanly without the unwanted `za` attack.
- Established the SE sequencer as a base for future sound work while reserving tempo, looping, channel allocation, and SE-priority mixing for a separate BGM driver.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: Non-blocking menu audio and staged race startup

- Added a non-blocking YM2151/OPM sound-effect driver on channel 7 for menu feedback without busy waits or delays in the game loop.
- Added a bright, rising metallic confirmation sound lasting approximately 0.25 seconds.
- Added a lower, descending cancellation sound lasting approximately 0.15 seconds so confirmation and cancellation are clearly distinguishable.
- Added a short science-fiction-style selection sound lasting approximately 0.1 seconds.
- Plays the selection sound only when the title player count or CPU level actually changes, and when the course selection moves left or right.
- Starts confirmation and cancellation sounds immediately when the input transition is detected, before mode finalization or preparation begins.
- Changed page presentation so a completed back page is displayed at the next VBlank instead of waiting for another VBlank inside `render()`.
- Keeps the previous front page visible while a new mode is being prepared, allowing VBlank handling and sound updates to continue normally.
- Added `GameMode::initialize_step()` so modes can divide expensive preparation across multiple VBlank intervals without rendering or updating gameplay early.
- Split the race trigonometric table generation into eight 32-entry steps instead of calculating all 256 floating-point entries in one call.
- Moved race camera setup and initialization of each car into separate preparation frames.
- Added an initialization failure result distinct from an incomplete preparation step.
- Fixed the title floor disappearing when the first displayed hero frame was not a cut frame by always drawing the complete static shot on a page's first render.
- Reduced the title garage clearing area so camera-cut cleanup no longer erases the `UP DOWN MODE  LEFT RIGHT LEVEL` instruction line.
- Preserved the existing fixed 20 Hz simulation, two-page graphics buffering, and precomputed race-introduction geometry.
- Successfully rebuilt `human.sys` and generated `dist/wdrfrace.xdf` with the WSL Ubuntu 24.04 elf2x68k toolchain.

## 2026-08-26: In-race sound effects and controller usability

- Added six high-priority in-race FM sound effects: BOOST, DRIFT, TACKLE, WALL, GATE, and SLIPSTREAM.
- Added event-priority handling so important collision sounds are not immediately masked by lower-priority events occurring in the same update.
- Added all new effects to the SOUND TEST label list.
- Disabled the SELECT sound while moving left or right inside SOUND TEST so auditioned effects remain easy to distinguish.
- Retuned SELECT as a short metallic cue and strengthened COUNTDOWN clarity.
- Retuned START toward a cleaner BOOST-like rising tone.
- Retuned DRIFT, TACKLE, WALL, and FINAL LAP to better match the futuristic racing presentation.
- Added controller cancellation: button 2 cancels menu screens, while buttons 1 and 2 together return from a race without conflicting with the brake control.
- Expanded HOW TO PLAY to explicitly show Q/N as BOOST, E/M as BRAKE, and the equivalent pad-button assignments.
- Successfully rebuilt with the Ubuntu 24.04 elf2x68k environment and regenerated `dist/wdrfrace.xdf`.