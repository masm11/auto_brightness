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
#include <unistd.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include "backlight.h"
#include "config.h"

static int dirfd = -1;
static int max_brightness = 1;
static volatile int goal = 1;

static void *backlight_loop(void *parm)
{
    while (1) {
	char buf[128];
	int fd;
	
	if ((fd = openat(dirfd, "brightness", O_RDONLY)) == -1) {
	    perror("brightness");
	    exit(1);
	}
	
	int s = read(fd, buf, sizeof buf);
	if (s == -1) {
	    perror("read");
	    exit(1);
	}
	if (s >= sizeof buf) {
	    fprintf(stderr, "out of buffer?\n");
	    exit(1);
	}
	close(fd);
	
	buf[s] = '\0';
	int now = atoi(buf);
	
	if (now != goal) {
	    if (now < goal) {
		if ((now += ADJUST_STEP) > goal)
		    now = goal;
	    } else {
		if ((now -= ADJUST_STEP) < goal)
		    now = goal;
	    }
	    
	    snprintf(buf, sizeof buf, "%d", now);
	    
	    if ((fd = openat(dirfd, "brightness", O_WRONLY)) == -1) {
		perror("brightness");
		exit(1);
	    }
	    
	    if (write(fd, buf, strlen(buf)) == -1) {
		perror("write");
		exit(1);
	    }
	    close(fd);
	}
	
	usleep(ADJUST_MSEC * 1000);
    }
    
    return NULL;
}

int backlight_init(void)
{
    if ((dirfd = open(BACKLIGHT_DIR, O_RDONLY | O_DIRECTORY)) == -1) {
	perror(BACKLIGHT_DIR);
	exit(1);
    }
    
    int fd;
    if ((fd = openat(dirfd, "max_brightness", O_RDONLY)) == -1) {
	perror("max_brightness");
	exit(1);
    }
    char buf[128];
    int s;
    if ((s = read(fd, buf, sizeof buf)) == -1) {
	perror("read");
	exit(1);
    }
    if (s >= sizeof buf) {
	fprintf(stderr, "out of buffer.\n");
	exit(1);
    }
    max_brightness = atoi(buf);
    close(fd);
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t thr;
    if (pthread_create(&thr, &attr, backlight_loop, NULL) != 0) {
	fprintf(stderr, "starting backlight thread failed.\n");
	exit(1);
    }
}

void backlight_set_ambient(int ambient)
{
#ifdef DEBUG
    fprintf(stderr, "ambient=%d.\n", ambient);
#endif
    int new_goal = (double) (BRIGHTNESS_1 - BRIGHTNESS_0) / (AMBIENT_1 - AMBIENT_0) * (ambient - AMBIENT_0) + BRIGHTNESS_0;
    if (new_goal > max_brightness)
	new_goal = max_brightness;
    if (new_goal < 1)
	new_goal = 1;
    goal = new_goal;
#ifdef DEBUG
    fprintf(stderr, "brightness=%d.\n", new_goal);
#endif
}
