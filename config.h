/*
    auto_brightness - adjust LCD brightness according to ambient.
    Copyright (C) 2016 Yuuki Harano

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONFIG_H__INCLUDED
#define CONFIG_H__INCLUDED

/* カメラのデバイス名 */
#define VIDEO_DEVICE	"/dev/video0"

/* カメラから取得する画像の幅と高さ */
#define WIDTH		640
#define HEIGHT		480

/* sysfs 内の backlight のある場所 */
#define BACKLIGHT_DIR	"/sys/class/backlight/intel_backlight"

/*
 * 周囲の明るさが AMBIENT_0 の時に brightness が BRIGHTNESS_0 になり、
 * 周囲の明るさが AMBIENT_1 の時に brightness が BRIGHTNESS_1 になる。
 */
#define AMBIENT_0	27
#define BRIGHTNESS_0	1
#define AMBIENT_1	62
#define BRIGHTNESS_1	468

/*
 * ADJUST_MSEC ミリ秒ごとに brightness を ADJUST_STEP ずつ変化させる。
 */
#define ADJUST_MSEC	10
#define ADJUST_STEP	1

// #define DEBUG 1

#endif	/* ifndef CONFIG_H__INCLUDED */
