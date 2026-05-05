#include "backend.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <cstring>
#include <cstdlib>

static int fbfd;
static uint8_t* fbmem = nullptr;

Framebuffer initFramebuffer() {
    fbfd = open("/dev/fb0", O_RDWR);

    fb_var_screeninfo v;
    fb_fix_screeninfo f;

    ioctl(fbfd, FBIOGET_VSCREENINFO, &v);
    ioctl(fbfd, FBIOGET_FSCREENINFO, &f);

    int w = v.xres;
    int h = v.yres;

    fbmem = (uint8_t*)mmap(nullptr, w * h,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED, fbfd, 0);

    return { fbmem, fbmem, w, h, (int)f.line_length };
}

void present(Framebuffer fb) {
    // full copy (safe baseline; optimize later)
    std::memcpy(fb.device, fb.data, fb.w * fb.h);

    // Kindle refresh trigger (temporary hack)
    system("eips ''");
}