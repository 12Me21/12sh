#include "types.h"
//50:43 BB
typedef unsigned int BucketIndex;

struct DictNode;

typedef struct DictNode {
	void* _value;
	Str key;
	struct DictNode* bnext;
	struct DictNode* snext;
	struct DictNode* sprev;
	Bool managed;
} DictNode;

typedef struct {
	DictNode** buckets;
	BucketIndex size;
	BucketIndex items;
	DictNode* shead;
	DictNode* stail;
} Dict;

Dict* Dict_init(BucketIndex buckets, DictNode* items);
void* Dict_get(Dict*, Str);
DictNode* Dict_add(Dict* tb, Str key);
Dict* Dict_new(BucketIndex x);
DictNode* Dict_remove(Dict* tb, Str key);
void Dict_free(Dict*);
