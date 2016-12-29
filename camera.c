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
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include "camera.h"
#include "config.h"

static int fd = -1;
static unsigned char *mem;
static volatile int ave = 0;

static int xioctl(int fd, int ctl, void *ptr)
{
    while (1) {
	int r = ioctl(fd, ctl, ptr);
	if (r == -1 && errno == EINTR)
	    continue;
	return r;
    }
}

static void *camera_loop(void *parm)
{
    time_t last = 0;
    
    while (1) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	if (select(fd + 1, &fds, NULL, NULL, NULL) == -1) {
	    perror("select");
	    exit(1);
	}
	
	struct v4l2_buffer buf = {
	    .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	    .memory = V4L2_MEMORY_MMAP,
	};
	if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
	    perror("ioctl(DQBUF)");
	    exit(1);
	}
	
	time_t now = time(NULL);
	if (now != last) {
	    last = now;
	    
	    unsigned long sum = 0;
	    for (int i = 0; i < WIDTH * HEIGHT * 2; i += 2)
		sum += mem[i];
	    ave = sum / (WIDTH * HEIGHT);
	}
	
	if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
	    perror("ioctl(QBUF)");
	    exit(1);
	}
    }
}

void camera_init(void)
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
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t thr;
    if (pthread_create(&thr, &attr, camera_loop, NULL) != 0) {
	fprintf(stderr, "starting camera thread failed.\n");
	exit(1);
    }
}

int camera_get_ambient(void)
{
    return ave;
}
