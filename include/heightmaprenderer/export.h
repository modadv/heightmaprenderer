#ifndef HEIGHTMAP_EXPORT_H
#define HEIGHTMAP_EXPORT_H

#ifdef HEIGHTMAP_SHARED_LIBS
    #ifdef _WIN32
        #ifdef HEIGHTMAP_BUILDING_DLL
            #define HEIGHTMAP_API __declspec(dllexport)
        #else
            #define HEIGHTMAP_API __declspec(dllimport)
        #endif
    #else
        #define HEIGHTMAP_API __attribute__((visibility("default")))
    #endif
#else
    #define HEIGHTMAP_API
#endif

#endif // HEIGHTMAP_EXPORT_H