#include "types.h"

typedef unsigned int BucketIndex;

struct DictNode;

typedef struct DictNode {
	Str key;
	Str value;
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
DictNode* Dict_get(Dict* tb, Str key);
DictNode* Dict_add(Dict* tb, Str key);
Dict* Dict_new(BucketIndex x);
DictNode* Dict_remove(Dict* tb, Str key);