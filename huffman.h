#include <stdlib.h>
#include <stdint.h>

typedef struct _TreeNode {
	unsigned char character;
	size_t frequency;
	struct _TreeNode* left;
	struct _TreeNode* right;
} TreeNode;

typedef struct _Queue {
   TreeNode* treenode;
   struct _Queue* next;
} Queue;

bool get_frqs(long* frqs, const char* path);
void write_frqs(long* frqs, const char* path);

void pq_push(Queue** start_a, TreeNode* tnode);
Queue* pq_pop(Queue** start_a);

unsigned char* read_file(const char* path, int* size);
long get_size(unsigned char* seq, int num_bytes, int start_pos);

TreeNode* make_tree(unsigned char* topology, int* location, int* byte_idx, long size, FILE* fptr);
void decode_tree(unsigned char* decode_traversal, int location, int byte_idx, TreeNode* root, long size, FILE* fptr, FILE* fptr2, long fsize);

Queue* make_tree_pq(long* freqs);
TreeNode* make_huff_tree(Queue* head);

void destroy_huff_tree(TreeNode** a_root);

void preorder_traverse(TreeNode* root, FILE* fptr);
void huff_bits(TreeNode* root, unsigned long* total_bits, int depth);