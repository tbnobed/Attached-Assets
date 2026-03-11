#include "DeckLinkAPI.h"

#include <dlfcn.h>
#include <pthread.h>
#include <cstdio>

#define kDeckLinkAPI_SO "/usr/lib/libDeckLinkAPI.so"

typedef IDeckLinkIterator*                  (*CreateIteratorFunc)(void);
typedef IDeckLinkAPIInformation*            (*CreateAPIInformationFunc)(void);
typedef IDeckLinkGLScreenPreviewHelper*     (*CreateOpenGLScreenPreviewHelperFunc)(void);
typedef IDeckLinkVideoConversion*           (*CreateVideoConversionFunc)(void);
typedef IDeckLinkDiscovery*                 (*CreateDiscoveryFunc)(void);

static void*                                gDeckLinkLib                = NULL;
static CreateIteratorFunc                   gCreateIteratorFunc         = NULL;
static CreateAPIInformationFunc             gCreateAPIInfoFunc          = NULL;
static CreateOpenGLScreenPreviewHelperFunc  gCreateOpenGLPreviewFunc    = NULL;
static CreateVideoConversionFunc            gCreateVideoConversionFunc  = NULL;
static CreateDiscoveryFunc                  gCreateDiscoveryFunc        = NULL;
static pthread_once_t                       gDeckLinkOnceControl        = PTHREAD_ONCE_INIT;

static void InitDeckLinkAPI(void)
{
    gDeckLinkLib = dlopen(kDeckLinkAPI_SO, RTLD_NOW | RTLD_GLOBAL);
    if (!gDeckLinkLib)
        return;

    gCreateIteratorFunc        = (CreateIteratorFunc)dlsym(gDeckLinkLib, "CreateDeckLinkIteratorInstance_0004");
    gCreateAPIInfoFunc         = (CreateAPIInformationFunc)dlsym(gDeckLinkLib, "CreateDeckLinkAPIInformationInstance_0003");
    gCreateOpenGLPreviewFunc   = (CreateOpenGLScreenPreviewHelperFunc)dlsym(gDeckLinkLib, "CreateOpenGLScreenPreviewHelper_0001");
    gCreateVideoConversionFunc = (CreateVideoConversionFunc)dlsym(gDeckLinkLib, "CreateVideoConversionInstance_0001");
    gCreateDiscoveryFunc       = (CreateDiscoveryFunc)dlsym(gDeckLinkLib, "CreateDeckLinkDiscoveryInstance_0003");

    if (!gCreateIteratorFunc)
        gCreateIteratorFunc = (CreateIteratorFunc)dlsym(gDeckLinkLib, "CreateDeckLinkIteratorInstance_0003");
}

bool IsDeckLinkAPIPresent(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    return (gDeckLinkLib != NULL && gCreateIteratorFunc != NULL);
}

IDeckLinkIterator* CreateDeckLinkIteratorInstance(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    if (gCreateIteratorFunc)
        return gCreateIteratorFunc();
    return NULL;
}

IDeckLinkAPIInformation* CreateDeckLinkAPIInformationInstance(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    if (gCreateAPIInfoFunc)
        return gCreateAPIInfoFunc();
    return NULL;
}

IDeckLinkGLScreenPreviewHelper* CreateOpenGLScreenPreviewHelper(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    if (gCreateOpenGLPreviewFunc)
        return gCreateOpenGLPreviewFunc();
    return NULL;
}

IDeckLinkVideoConversion* CreateDeckLinkVideoConversionInstance(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    if (gCreateVideoConversionFunc)
        return gCreateVideoConversionFunc();
    return NULL;
}

IDeckLinkDiscovery* CreateDeckLinkDiscoveryInstance(void)
{
    pthread_once(&gDeckLinkOnceControl, InitDeckLinkAPI);
    if (gCreateDiscoveryFunc)
        return gCreateDiscoveryFunc();
    return NULL;
}
