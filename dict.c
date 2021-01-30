#include "dict.h"
#include <stdlib.h>
#include <string.h>

Dict* Dict_new(BucketIndex size) {
	Dict* new = malloc(sizeof(Dict));
	new->shead = NULL;
	new->size = size;
	new->buckets = malloc(size*sizeof(DictNode*));
	BucketIndex i;
	for (i=0;i<size;i++)
		new->buckets[i] = NULL;
	return new;
}

static BucketIndex hash(Dict* dict, Str str){
	BucketIndex hash = 5381;
	for(; *str; str++)
		hash = hash*33 + *str;
	return hash % dict->size;
}

DictNode* Dict_get(Dict* tb, Str key) {
	BucketIndex index = hash(tb, key);
	DictNode* bucket = tb->buckets[index];
	for (; bucket; bucket = bucket->bnext) {
		if (strcmp(bucket->key, key)==0)
			return bucket;
	}
	return NULL;
}

DictNode* Dict_add(Dict* tb, Str key) {
	BucketIndex index = hash(tb, key);
	DictNode* bucket = tb->buckets[index];
	for (; bucket; bucket = bucket->bnext)
		if (strcmp(bucket->key, key)==0)
			return bucket;
	bucket = malloc(sizeof(DictNode));
	bucket->bnext = tb->buckets[index];
	bucket->key = key;
	tb->buckets[index] = bucket;
	return bucket;
}

Dict* Dict_init(BucketIndex buckets, DictNode* items) {
	Dict* new = Dict_new(buckets);
	for (; items->key; items++) {
		BucketIndex index = hash(new, items->key);
		items->bnext = new->buckets[index];
		new->buckets[index] = items;
	}
	return new;
}
