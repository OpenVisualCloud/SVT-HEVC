/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#ifndef EbObject_h
#define EbObject_h
#include <stdlib.h>

#include "EbMalloc.h"

/*typical usage like this.

  typedef A {
      EbDctor dctor;
      //...
  };

  a_dctor(void* p)
  {
      A* a = (A*)p;
      //free everything allocated by A;
  }

  a_ctor(A* a)
  {
      //this not need if you do not need a deconsturctor.
      a->dctor = a_dctor;
      //consturct everything
  }

  A* o;
  EB_NEW(o, a_ctor);
  //...
  EB_RELEASE(o);

}
*/

typedef void(*EbDctor)(void* pobj);

#define EB_DELETE_UNCHECKED(pobj) \
    do { \
        if ((pobj)->dctor) (pobj)->dctor(pobj); \
        EB_FREE((pobj)); \
    } while (0)

//trick: to support zero param constructor
#define EB_VA_ARGS(...) ,##__VA_ARGS__

#define EB_NO_THROW_NEW(pobj, ctor, ...) \
   do { \
        EB_ERRORTYPE err; \
        size_t size = sizeof(*pobj); \
        EB_NO_THROW_CALLOC(pobj, 1, size); \
        if (pobj) { \
            err = ctor(pobj EB_VA_ARGS(__VA_ARGS__)); \
            if (err != EB_ErrorNone) EB_DELETE_UNCHECKED(pobj); \
        } \
    } while (0)

#define EB_NEW(pobj, ctor, ...) \
    do { \
        EB_ERRORTYPE err; \
        size_t size = sizeof(*pobj); \
        EB_CALLOC(pobj, 1, size); \
        err = ctor(pobj EB_VA_ARGS(__VA_ARGS__)); \
        if (err != EB_ErrorNone) { \
            EB_DELETE_UNCHECKED(pobj); \
            return err; \
        } \
    } while (0)

#define EB_DELETE(pobj) \
    do { \
        if (pobj) EB_DELETE_UNCHECKED(pobj); \
    } while (0)

#define EB_DELETE_PTR_ARRAY(pa, count) \
    do {\
        uint32_t i; \
        if (pa) { \
            for (i = 0; i < count; i++) { \
                EB_DELETE(pa[i]); \
            } \
            EB_FREE(pa); \
        } \
    } while (0)

#undef EB_DELETE_UNCHECK //do not use this outside
//#undef EB_VA_ARGS

#endif //EbObject_h
