#include "types.h"

typedef unsigned int BucketIndex;

struct DictNode;

typedef struct DictNode {
	Str key;
	Str value;
	struct DictNode *bnext;
} DictNode;

typedef struct {
	DictNode *shead;
	DictNode **buckets;
	BucketIndex size;
} Dict;

Dict* Dict_init(BucketIndex buckets, DictNode* items);
DictNode* Dict_get(Dict* tb, Str key);
DictNode* Dict_add(Dict* tb, Str key);
Dict* Dict_new(BucketIndex x);
