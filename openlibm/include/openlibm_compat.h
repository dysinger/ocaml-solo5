#ifndef OPENLIBM_COMPAT_H
#define OPENLIBM_COMPAT_H

#ifndef __strong_alias
#define __strong_alias(new, old) \
    extern typeof(old) new __attribute__((alias(#old)))
#endif

#endif