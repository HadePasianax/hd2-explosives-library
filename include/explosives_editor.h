/* Experimental optional editor bridge. ABI 1 of the gameplay API is unchanged. */
#ifndef EXPLOSIVES_EDITOR_H
#define EXPLOSIVES_EDITOR_H
#include <stdint.h>
typedef struct ElEditorRecord {uint32_t size,id,donor,damage;char name[128];} ElEditorRecord;
typedef struct ElEditorApi {
 uint32_t size,abi;
 /* Thread-safe snapshots. Enumeration returns 0 past the currently registered end. */
 int (*record)(uint32_t index,ElEditorRecord* out);
 int (*read)(uint32_t id,uint32_t offset,float* now,float* base);
 /* Only radii (+16/+20/+24). NULL resets to registration-time value.
  * Accepted writes are queued; the game thread validates and applies them. */
 int (*set)(uint32_t id,uint32_t offset,const float* value);
} ElEditorApi;
#endif
