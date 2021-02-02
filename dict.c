#include "dict.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// todo: make memory management better
// ideas:
// - all items in dict are malloced/freed by dict
///  problem: directly setting node->value not allowed. also inefficient
// - flag to track which items are managed by dict
//   problem: now other programs may have to free some stuff and nnnX

Dict* Dict_new(BucketIndex size) {
	Dict* new = malloc(sizeof(Dict));
	
	new->buckets = malloc(size*sizeof(DictNode*));
	new->size = size;
	new->items = 0;
	BucketIndex i;
	for (i=0; i<size; i++)
		new->buckets[i] = NULL;
	
	new->shead = NULL;
	new->stail = NULL;
	
	return new;
}

void Dict_free(Dict* dict) {
	DictNode* node;
	for (node=dict->shead; node; ) {
		//printf("freed %s\n", node->key);
		free(node->key);
		free(node->value);
		DictNode* prev = node;
		node = node->snext;
		free(prev);
	}
	free(dict->buckets);
	free(dict);
}

static BucketIndex hash(Dict* dict, Str str){
	BucketIndex hash = 5381;
	for(; *str; str++)
		hash = hash*33 + *str;
	return hash % dict->size;
}

DictNode* Dict_get(Dict* tb, Str key) {
	BucketIndex index = hash(tb, key);
	DictNode* node = tb->buckets[index];
	for (; node; node = node->bnext)
		if (strcmp(node->key, key)==0)
			return node;
	return NULL;
}

DictNode* Dict_remove(Dict* tb, Str key) {
	// I don't reuse Dict_get here, because we also need the previous node and the bucket index
	BucketIndex index = hash(tb, key);
	DictNode* node = tb->buckets[index];
	DictNode* prev = NULL;
	for (; node; prev = node, node = node->bnext)
		if (strcmp(node->key, key)==0) {
			tb->items--;
			// remove from bucket
			if (prev)
				prev->bnext = node->bnext;
			else
				tb->buckets[index] = node->bnext;
			// remove from sorted list
			*(node->sprev ? &node->sprev->snext : &tb->shead) = node->snext;
			*(node->snext ? &node->snext->sprev : &tb->stail) = node->sprev;
			free(node);
			return node; //this pointer won't be valid anymore, but it's a useful truthy return value
		}
	return NULL;
}

DictNode* Dict_add(Dict* tb, Str key) {
	BucketIndex index = hash(tb, key);
	DictNode* node = tb->buckets[index];
	for (; node; node = node->bnext)
		if (strcmp(node->key, key)==0)
			return node;
	// Create new node
	node = malloc(sizeof(DictNode));
	tb->items++;
	// Add to bucket
	node->bnext = tb->buckets[index];
	node->key = strdup(key);
	tb->buckets[index] = node;
	// Add to end of sorted list
	node->snext = NULL;
	node->sprev = tb->stail;
	if (!tb->shead) tb->shead = node;
	if (tb->stail)	tb->stail->snext = node;
	tb->stail = node;
	
	return node;
}

// Initialize a dictionary from a list of nodes
//TODO: currently, trying to delete these items will break if they arent allocated with malloc
Dict* Dict_init(BucketIndex buckets, DictNode* items) {
	Dict* new = Dict_new(buckets);
	new->shead = items;
	for (; items->key; items++) {
		new->items++;
		BucketIndex index = hash(new, items->key);
		
		items->bnext = new->buckets[index];
		new->buckets[index] = items;
		
		items->sprev = new->stail;
		items->snext = NULL;
		new->stail->snext = items;
		new->stail = items;
	}
	return new;
}
