# Galton Board Simulator

ゴルトンボード（釘板）の物理シミュレーターです。ボールがピンに衝突しながら落下し、二項分布（正規分布への収束）をリアルタイムで視覚化します。

## 機能

- **物理シミュレーション** — 重力・反発係数・空気抵抗を考慮したボール落下
- **3種類のピン配置** — 三角形 / 四角格子 / ひし形
- **三角形配置の台形物理モデル** — 壁の影響を排除するため物理的には台形配置（後述）
- **理論分布曲線** — B(n, p) の理論値をベジエ曲線でヒストグラムにオーバーレイ表示
- **右確率 p の調整** — p ≠ 0.5 にすることで非対称二項分布を観察可能
- **リアルタイム統計** — 落下総数・実測平均・実測標準偏差
- **シミュレーション速度** — 1x / 2x / 5x / 10x（ボール投入速度には影響しない）
- **パラメータ保存** — 設定を保存して次回起動時に自動復元

## 調整可能なパラメータ

| パラメータ | 範囲 | デフォルト |
|-----------|------|-----------|
| ピンの段数 | 5 〜 18 段 | 10 段 |
| 投入速度 | 1 〜 50 個/秒 | 5 個/秒 |
| 重力 | 100 〜 800 px/s² | 350 px/s² |
| ピンの弾性 | 0.00 〜 0.85 | 0.50 |
| 右確率 p | 1% 〜 99% | 50% |
| ボールサイズ | 0.50x 〜 2.00x | 1.00x |

## Web 版（Qt for WebAssembly）

デスクトップ版と同じ Qt アプリを WebAssembly にビルドして GitHub Pages で公開しています：
**https://nanamitm.github.io/galton-board-simulator/**

- 機能はデスクトップ版と同じ
- パラメータはブラウザの `localStorage` に保存されます（「現在値を保存」で保存）
- 表示領域が狭いときはコントロールパネルを上、シミュレーションを下に積み替えます
- 初回ロードは gzip 後で約 5 MB

スレッドを使っていないためシングルスレッド版 Qt でビルドでき、`SharedArrayBuffer`
（＝COOP/COEP ヘッダ）は不要です。

**ビルドに必要なもの**
- Qt 6.11.1 の WebAssembly 版（`wasm_singlethread`）＋同バージョンのホスト Qt
- Emscripten 4.0.7（この Qt がビルドに使ったバージョン。他だと拒否されます）

```bash
source /path/to/emsdk/emsdk_env.sh
/path/to/Qt/6.11.1/wasm_singlethread/bin/qt-cmake -S . -B build-wasm   -DCMAKE_BUILD_TYPE=Release   -DQT_HOST_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build-wasm
python -m http.server 8080 --directory build-wasm   # /index.html を開く
```

`.github/workflows/pages.yml` が `master` への push でビルドとデプロイを行います。
公開物は `wasm/make-dist.py` が組み立て、ファイル名に内容ハッシュを付けます
（GitHub Pages が `Cache-Control: max-age=600` を返すため、新しい `index.html`
と古い wasm が混ざるのを防ぐため）。

### 日本語フォント

Qt for WebAssembly は DejaVu しか同梱しておらず CJK が豆腐になるため、
`resources/fonts/NotoSansJP-subset.ttf`（UI で使う約 435 文字だけに絞った
Noto Sans JP Regular・約 89 KB、SIL Open Font License 1.1）を埋め込んでいます。
日本語の文言を追加したら再生成してください：

```bash
python -m pip install fonttools
python tools/subset_font.py path/to/NotoSansJP.ttf
```

---

## 動作環境

- Windows 10 / 11
- Qt 6.x (Widgets)
- CMake 3.16 以上
- C++17 対応コンパイラ（MinGW 13 / MSVC 2022）

## ビルド方法

```bash
# 構成
cmake -S . -B build -G Ninja

# ビルド
cmake --build build

# 実行
build\GaltonBoardSimulator.exe
```

Qt が標準パスにない場合は `CMAKE_PREFIX_PATH` を指定してください。

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"
```

## プロジェクト構成

```
galton-board-simulator/
├── src/
│   ├── main.cpp
│   ├── mainwindow.h / .cpp      # メインウィンドウ・UIコントロール
│   └── simulationwidget.h / .cpp # 物理シミュレーション・描画
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## 技術的な補足

- ピン衝突後のx速度は50/50（またはp確率）のコインフリップで決定し、理論的な二項分布に収束させています
- 理論曲線はCatmull-Rom → Cubic Bezierへの変換により滑らかに描画しています
- パラメータはQSettingsを使用してWindowsレジストリに保存されます

### 三角形配置の台形物理モデル

実際のゴルトンボードと同様に、三角形配置では各行の両端にバッファピンを追加した台形配置を物理エンジン内で使用しています。これにより、端のピンに到達したボールが壁に反射して分布が歪む問題を防いでいます。バッファピンは半透明で表示され、中央の三角形部分のみ通常表示されます。

### シミュレーション速度とボール投入速度の独立性

シミュレーション速度（1x〜10x）は物理演算の繰り返し回数のみを制御します。ボールの投入速度は実時間ベースで維持されるため、「投入速度 5個/秒・シミュ速度 10x」と設定した場合でも、実際に投入されるのは 5個/秒のままです。
