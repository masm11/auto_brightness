# 画面輝度自動調整

## はじめに

周囲の明るさに応じて画面の明るさを自動調整するプログラムです。

## インストール方法と調整方法

調整はプログラムに埋め込みます。

まず、config.h にある、

```c
// #define DEBUG 1
```

を

```c
#define DEBUG 1
```

に書き換えてください。そして make します。

```
make
```

そのまま実行します。

```
./auto_brightness
```

すると、1秒おきに以下のように出力されます。

```
ambient=25.
brightness=1.
```

ambient がカメラから取得した明るさです。

auto_brightness を Ctrl+c キーで終了させ、
現在の ambient の値を config.h の

```c
#define AMBIENT_0	27
```

に設定し、もう一度 make します。

```
make clean
make
```

再度実行し、

```
./auto_brightness
```

もっと明るい方が良ければ、config.h の

```c
#define BRIGHTNESS_0	1
```

の値を大きくし、暗い方が良ければ小さくします(逆のハードウェアもあるようです)。

現在の周囲の明るさと画面の輝度に満足できたら、
今より明るい場所または暗い場所に移動します。

同じように、今度は

```c
#define AMBIENT_1	62
#define BRIGHTNESS_1	468
```

を修正して調整します。

うまくいったら、config.h の最初に変更した、

```c
#define DEBUG 1
```

を以下のように戻します。

```c
// #define DEBUG 1
```

そして make します。

```
make clean
make
```

完成した auto_brightness を PATH の通った場所に置き、
お使いの環境で自動起動するように設定してください。

## 実行できない場合

### /dev/video0 が open できない場合

```
% ls -al /dev/video0 
crw-rw----+ 1 root video 81, 0 12月 29 20:35 /dev/video0
% 
```

このようになっている場合は video グループに自分を追加してください。

### /sys/class/backlight/*/brightness が open できない場合

同梱の 99-backlight-intel_backlight.rules を自分の環境に併せて修正し、
/etc/udev/rules.d/ に置いて、OS を一旦 reboot してください。

グループに write permission が出ますので、自分をグループに追加してください。

## 輝度調整について

カメラからは随時映像が送られてきますが、
周囲の明るさの計算は1秒ごとです。

画面輝度の調整速度は、config.h の中の

```c
#define ADJUST_MSEC	10
#define ADJUST_STEP	1
```

です。

## ライセンス

GPL3 です。

## 作者

原野裕樹 &lt;masm@masm11.ddo.jp&gt;
