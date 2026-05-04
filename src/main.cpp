// leafy_fb.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

static unsigned char *fb;
static int fbfd;
static long screensize;
static int width, height, bpp, line_length;

void set_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    long location = (x * (bpp / 8)) + (y * line_length);

    fb[location] = b;
    fb[location + 1] = g;
    fb[location + 2] = r;
}

// simple fill rect
void draw_rect(int x0, int y0, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            set_pixel(x, y, r, g, b);
        }
    }
}

// simple triangle (filled-ish)
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3) {
    // naive scanline fill approximation (good enough for prototype)
    int minx = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
    int maxx = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
    int miny = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
    int maxy = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            // barycentric-ish hack (simple inclusion test)
            int w1 = (x2 - x1)*(y - y1) - (y2 - y1)*(x - x1);
            int w2 = (x3 - x2)*(y - y2) - (y3 - y2)*(x - x2);
            int w3 = (x1 - x3)*(y - y3) - (y1 - y3)*(x - x3);

            if ((w1 >= 0 && w2 >= 0 && w3 >= 0) ||
                (w1 <= 0 && w2 <= 0 && w3 <= 0)) {
                set_pixel(x, y, 0, 0, 0);
            }
        }
    }
}

// VERY primitive bitmap text (just blocks)
void draw_text_block(int x, int y, const char *msg) {
    for (int i = 0; msg[i]; i++) {
        for (int py = 0; py < 6; py++) {
            for (int px = 0; px < 4; px++) {
                if ((px + py) % 2 == 0) {
                    set_pixel(x + i*6 + px, y + py, 0, 0, 0);
                }
            }
        }
    }
}

void clear_screen() {
    memset(fb, 255, screensize); // white
}

int main() {
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("fb open");
        return 1;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);

    width = vinfo.xres;
    height = vinfo.yres;
    bpp = vinfo.bits_per_pixel;
    line_length = finfo.line_length;

    screensize = width * height * bpp / 8;

    fb = (unsigned char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    if (fb == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // ----------------------------
    // 2. black rectangle center
    // ----------------------------
    int rw = width / 3;
    int rh = height / 3;
    int rx = (width - rw) / 2;
    int ry = (height - rh) / 2;

    draw_rect(rx, ry, rw, rh, 0, 0, 0);

    // // Kindle partial refresh trigger (approximate)
    system("eips 'rect drawn'");

    sleep(1);

    clear_screen();

    // ----------------------------
    // 4. triangle
    // ----------------------------
    draw_triangle(width/2, height/3,
                  width/3, 2*height/3,
                  2*width/3, 2*height/3);

    system("eips 'triangle drawn'");
    // system("eips -f"); // full refresh

    sleep(1);

    clear_screen();

    // ----------------------------
    // 6. text (manual)
    // ----------------------------
    draw_text_block(width/2 - 40, height/2, "LEAFY");

    system("eips 'text drawn'");
    // system("eips 'done'");

    munmap(fb, screensize);
    close(fbfd);

    return 0;
}