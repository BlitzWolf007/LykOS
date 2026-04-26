#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    printf("HELLO");
    fflush(stdout);

    int fb = open("/dev/fb", O_WRONLY);
    if (fb < 0)
    {
        perror("open");
        return 1;
    }

    size_t fb_size = 1024 * 768 * 4;
    uint32_t *pixels = mmap(NULL, fb_size, PROT_WRITE, MAP_SHARED, fb, 0);
    if (pixels == MAP_FAILED)
    {
        perror("mmap");
        close(fb);
        return 1;
    }

    for (size_t i = 0; i < fb_size / 4; i++)
        pixels[i] = 0x00FF0000;

    munmap(pixels, fb_size);
    close(fb);
    return 0;
}
