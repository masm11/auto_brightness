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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include "config.h"
#include <X11/X.h>
#define XLIB_ILLEGAL_ACCESS
#include <X11/Xlib.h>

#define BYTES_PER_SCANLINE	(WIDTH * 4)
static int fd = -1;
static unsigned char *mem;
static Display *dpy;
static Window win, win2;
static GC gc;
static XImage *img, *img2;
static unsigned char image_data[BYTES_PER_SCANLINE * HEIGHT], image2_data[256 * 4 * 256];

static int xioctl(int fd, int ctl, void *ptr)
{
    while (1) {
	int r = ioctl(fd, ctl, ptr);
	if (r == -1 && errno == EINTR)
	    continue;
	return r;
    }
}

static void camera_loop(void)
{
    int xfd = dpy->fd;
    
    while (1) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	FD_SET(xfd, &fds);
	
	int fdw = -1;
	if (fdw < fd)
	    fdw = fd;
	if (fdw < xfd)
	    fdw = xfd;
	fdw++;
	
	if (select(fdw, &fds, NULL, NULL, NULL) == -1) {
	    perror("select");
	    exit(1);
	}
	
	if (FD_ISSET(xfd, &fds)) {
	    XProcessInternalConnection(dpy, xfd);
	    
	    XEvent ev;
	    while (XCheckWindowEvent(dpy, win, ~0, &ev))
		/*NOP*/ ;
	}
	
	if (FD_ISSET(fd, &fds)) {
	    struct v4l2_buffer buf = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
	    };
	    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
		perror("ioctl(DQBUF)");
		exit(1);
	    }
	    
	    unsigned long sum = 0;
	    for (int i = 0; i < WIDTH * HEIGHT * 2; i += 2)
		sum += mem[i];
	    int ave = sum / (WIDTH * HEIGHT);
	    fprintf(stderr, "%d\n", ave);
	    
	    unsigned int hist[256] = { 0, };
	    memset(hist, 0, sizeof hist);
	    
	    unsigned char *q = mem;
	    for (int y = 0; y < HEIGHT; y++) {
		unsigned char *p = &image_data[BYTES_PER_SCANLINE * y];
		for (int x = 0; x < WIDTH; x++) {
		    unsigned char b = *q;
		    q += 2;
		    *p++ = b;
		    *p++ = b;
		    *p++ = b;
		    *p++ = 0xff;
		    
		    hist[b]++;
		}
	    }
	    
	    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
		perror("ioctl(QBUF)");
		exit(1);
	    }
	    
	    for (int i = 0; i < 256; i++) {
		while (hist[i] >= 256) {
		    for (int j = 0; j < 256; j++)
			hist[j] >>= 1;
		}
	    }
	    {
		unsigned char *p = image2_data;
		for (int y = 0; y < 256; y++) {
		    for (int x = 0; x < 256; x++) {
			unsigned char b = hist[x] >= (256 - y) ? 0xff : 0x00;
			if (b == 0x00) {
			    if (x == ave)
				b = 0x44;
			}
			*p++ = b;
			*p++ = b;
			*p++ = b;
			*p++ = 0;
		    }
		}
	    }
	    
	    XPutImage(dpy, win, gc, img, 0, 0, 0, 0, WIDTH, HEIGHT);
	    XPutImage(dpy, win2, gc, img2, 0, 0, 0, 0, 256, 256);
	    XFlush(dpy);
	}
    }
}

static void camera_init(void)
{
    if ((fd = open(VIDEO_DEVICE, O_RDWR)) == -1) {
	perror(VIDEO_DEVICE);
	exit(1);
    }
    
    struct v4l2_capability cap;
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
	perror("ioctl(QUERY_CAP)");
	exit(1);
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
	fprintf(stderr, "not a video capture device.\n");
	exit(1);
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
	fprintf(stderr, "streaming not supported.\n");
	exit(1);
    }
    
    struct v4l2_cropcap cropcap = {
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
    };
    if (xioctl(fd, VIDIOC_CROPCAP, &cropcap) == -1) {
	perror("ioctl(CROPCAP)");
	exit(1);
    } else {
	struct v4l2_crop crop = {
	    .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	    .c = cropcap.defrect,
	};
	if (xioctl(fd, VIDIOC_S_CROP, &crop) == -1) {
#if 0
	    perror("ioctl(S_CROP)");
	    exit(1);
#endif
	}
    }
    
    struct v4l2_format fmt = {
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	.fmt = {
	    .pix = {
		.width = WIDTH,
		.height = HEIGHT,
		.pixelformat = V4L2_PIX_FMT_YUYV,
	    },
	},
    };
    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
	perror("ioctl(S_FMT)");
	exit(1);
    }
    if (fmt.fmt.pix.width != WIDTH || fmt.fmt.pix.height != HEIGHT) {
	fprintf(stderr, "%dx%d not supported.\n", WIDTH, HEIGHT);
	exit(1);
    }
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
	fprintf(stderr, "YUYV format not supported.\n");
	exit(1);
    }
    
    struct v4l2_requestbuffers req = {
	.count = 1,
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	.memory = V4L2_MEMORY_MMAP,
    };
    
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
	perror("ioctl(REQBUFS)");
	exit(1);
    }
    
    struct v4l2_buffer buf = {
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	.memory = V4L2_MEMORY_MMAP,
	.index = 0,
    };
    
    if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
	perror("ioctl(QUERYBUF)");
	exit(1);
    }
    
    if ((mem = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset)) == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
    
    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
	perror("ioctl(QBUF)");
	exit(1);
    }
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
	perror("ioctl(STREAMON)");
	exit(1);
    }
}

int main(void)
{
    if ((dpy = XOpenDisplay(NULL)) == NULL) {
	fprintf(stderr, "can't open display.\n");
	exit(1);
    }
    
    Screen *scr = DefaultScreenOfDisplay(dpy);
    Visual *vis = DefaultVisualOfScreen(scr);
    int depth = DefaultDepthOfScreen(scr);
    fprintf(stderr, "depth=%d.\n", depth);
    img = XCreateImage(dpy, vis, depth, ZPixmap, 0, (char *) image_data, WIDTH, HEIGHT, 32, BYTES_PER_SCANLINE);
    img2 = XCreateImage(dpy, vis, depth, ZPixmap, 0, (char *) image2_data, 256, 256, 32, 256 * 4);
    
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, WIDTH, HEIGHT, 0, 0, 0);
    XStoreName(dpy, win, "Camera Image");
    XMapWindow(dpy, win);
    
    win2 = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 256, 256, 0, 0, 0);
    XStoreName(dpy, win2, "Histgram");
    XMapWindow(dpy, win2);
    
    gc = XCreateGC(dpy, win, 0, NULL);
    XSetFunction(dpy, gc, GXcopy);
    
    camera_init();
    camera_loop();
}
