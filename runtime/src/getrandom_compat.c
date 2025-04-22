#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>  // Option 1: include this for ssize_t

EM_JS(int, emscripten_getrandom, (void *buf, size_t buflen), {
    try {
        let buffer = HEAPU8.subarray(buf, buf + buflen);
        self.crypto.getRandomValues(buffer);
        return buflen;
    } catch (e) {
        return -1;
    }
});

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags) {
    ssize_t result = emscripten_getrandom(buf, buflen);
    if (result == -1) {
        errno = ENOSYS;
    }
    return result;
}
#endif
