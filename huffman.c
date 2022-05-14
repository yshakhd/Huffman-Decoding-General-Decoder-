#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "huffman.h"

/*******HELPER FUNCTIONS*******/

static unsigned char get_char(unsigned char* topology, int* location, int* byte_idx) {
	unsigned char ch;
	if (*location) {
		unsigned char temp = topology[*byte_idx] >> *location;
		(*byte_idx)++;
		unsigned char temp2 = topology[*byte_idx] << (8 - *location);
		ch = temp | temp2;
	}
	else {
		ch = topology[*byte_idx];
		(*byte_idx)++;
	}
	return ch;
}

static unsigned char get_bit(unsigned char* topology, int location, int byte_idx) {
	return ((1 << location) & topology[byte_idx]) >> location;
}

static int cmp_frq(TreeNode* n1, TreeNode* n2) {
	int diff = n1 -> frequency - n2 -> frequency;
	if(!diff) {
		if(n1 -> character != '\0' && n2 -> character != '\0') {
			diff = n1 -> character - n2 -> character;
		}
		else {
			diff = -1;
		}
	}
	return diff;
}

static Queue* get_insert_location(Queue* pushloc, TreeNode* tnode) {
	while((pushloc -> next) && cmp_frq(pushloc -> next -> treenode, tnode) < 0) {
			pushloc = pushloc -> next;
	}
	return pushloc;
}

static bool is_node_larger(Queue** start_a, TreeNode* tnode) {
	if((*start_a) && cmp_frq((*start_a) -> treenode, tnode) < 0)
		return true;
	else
		return false;
}

static TreeNode* create_tnode(unsigned char ch, size_t frq, TreeNode* left, TreeNode* right) {
	TreeNode* tnode = malloc(sizeof(*tnode));
	tnode -> character = ch;
	tnode -> frequency = frq;
	tnode -> left = left;
	tnode -> right = right;
	return tnode;
}

Queue* create_queue_node(TreeNode* tnode) {
	Queue* new_node = malloc(sizeof(*new_node));
	new_node -> treenode = tnode;
	return new_node;
}

/*******ENDOFHELPERFUNCTIONS*******/

void pq_push(Queue** start_a, TreeNode* tnode) {
	Queue* new_node = create_queue_node(tnode);
	if(is_node_larger(start_a, tnode)) { // if tnode is larger than head
		Queue* pushloc = *start_a;
		pushloc = get_insert_location(pushloc, tnode); // find where in the queue it should go
		new_node -> next = pushloc -> next;
		pushloc -> next = new_node;
	} else {
		new_node -> next = *start_a; 
		*start_a = new_node; // make newnode head
	}
}

Queue* pq_pop(Queue** start_a) {
	Queue* new_node = *start_a;	
	if(*start_a) {
		*start_a = (*start_a) -> next;
		new_node -> next = NULL;
	}
	return new_node;
}

long get_size(unsigned char * seq, int num_bytes, int start_pos) {
	unsigned long num = 0;
	int ctr = 0;
	int idx = start_pos;;
	while(ctr < num_bytes) {
		num = (seq[idx] << (ctr * num_bytes)) | num;
		idx++;
		ctr++;
	}
	return num;
}

unsigned char* read_file(const char* path, int* size) {
	FILE * fptr = fopen(path, "rb");
	if(!fptr){
		return NULL;
	}
	fseek(fptr, 0, SEEK_END);
	int num_bytes = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);
	int len = num_bytes;
	*size = len;
	if(!num_bytes) {
		fclose(fptr);
		return NULL;
	}
	unsigned char* seq = malloc(num_bytes);
	fread(seq, sizeof(char), num_bytes, fptr);
	fclose(fptr);
	return seq;
}

TreeNode* make_tree(unsigned char* topology, int* location, int* byte_idx, long size, FILE* fptr) {
	if(*byte_idx >= size)
		return NULL;
	unsigned char bit = get_bit(topology, *location, *byte_idx);
	unsigned char num = bit + '0';
	fwrite(&num, sizeof(char), 1, fptr);
	if((*location)++ == 7) {
			*location = 0;
			(*byte_idx)++;
	}
	if(bit) {
		unsigned char ch = get_char(topology, location, byte_idx);
		fwrite(&ch, sizeof(char), 1, fptr);
		return create_tnode(ch, 0, NULL, NULL);
	}
	TreeNode * node = malloc(sizeof(*node));
	node -> left = make_tree(topology, location, byte_idx, size, fptr);
	node -> right = make_tree(topology, location, byte_idx, size, fptr);
	return node;
}

void decode_tree(unsigned char* decode_traversal, int location, int byte_idx, TreeNode* root, long size, FILE* fptr, FILE* fptr2, long fsize) {
	TreeNode* curr_node = root;
	long char_num = 0; // determine number of bits used to encode original message
	for(long i = 0; i < size * 8; i++) {
        unsigned char bit = get_bit(decode_traversal, location, byte_idx);
		if(location++ == 7) {
			location = 0;
			byte_idx++;
		}
		// 0 is left and 1 is right
		if (!bit)
           curr_node = curr_node -> left;
        else
           curr_node = curr_node -> right;
  
        // leaf node
        if (!(curr_node -> left) && !(curr_node -> right))
		{
            fwrite(&(curr_node -> character), sizeof(char), 1, fptr);
            curr_node = root;
			char_num++;
		}
		if(char_num == fsize) {
			long num_bytes = (i + 1) / 8; // number of bytes
			int remaining_bits = (i + 1) % 8; // number of bits remaining
			fwrite(&num_bytes, sizeof(long), 1, fptr2);
			fwrite(&remaining_bits, sizeof(int), 1, fptr2);
			break;
		}
	}
	if(!char_num) {
		long zero_bytes = 0;
		int zero_bits = 0;
		fwrite(&zero_bytes, sizeof(long), 1, fptr2);
		fwrite(&zero_bits, sizeof(int), 1, fptr2);
	}
}

Queue* make_tree_pq(long* frqs) {
	Queue* head = NULL;
	int f_idx = 0;
	while(f_idx < 256) {
		if(frqs[f_idx]) {
			TreeNode* tnode = create_tnode(f_idx, frqs[f_idx], NULL, NULL);
			pq_push(&head, tnode);
		}
		f_idx++;
	}
	return head;
}

TreeNode* make_huff_tree(Queue* head) {
	if(!head)
		return NULL;
	while(head -> next) {
		Queue* node1 = pq_pop(&head);
		Queue* node2 = pq_pop(&head);
		TreeNode* new_treenode = create_tnode('\0', node1 -> treenode -> frequency + node2 -> treenode -> frequency, node1 -> treenode, node2 -> treenode);
		pq_push(&head, new_treenode);
		free(node1);
		free(node2);
	}
	TreeNode* root = head -> treenode;
	free(head);
	return root;
}

void preorder_traverse(TreeNode* root, FILE* fptr) {
 	while(root) {
		unsigned char byte; 
		if(root -> left || root -> right) {
			byte = '0';
			fwrite(&byte, sizeof(char), 1, fptr);
		} else {
			byte = '1';
			fwrite(&byte, sizeof(char), 1, fptr);
			fwrite(&(root -> character), sizeof(char), 1, fptr);
		}
		preorder_traverse(root -> left, fptr);
		root = root -> right;
	}
}

void huff_bits(TreeNode* root, unsigned long* total_bits, int depth) {
	if(root) { 
		if(!(root -> left) && !(root -> right)) {
			*total_bits += depth * root -> frequency;
		}
		else {
			huff_bits(root -> left, total_bits, depth + 1);
			huff_bits(root -> right, total_bits, depth + 1);
		}
	}
}

void destroy_huff_tree(TreeNode** a_root) {
	if(*a_root) {
		destroy_huff_tree(&((*a_root) -> left));
		destroy_huff_tree(&((*a_root) -> right));
		free(*a_root);
		*a_root = NULL;
	}
}

bool get_frqs(long* frqs, const char* path) {
	FILE* file = fopen(path, "r");
	if(!file)
		return false;
	else {
		unsigned char ch = fgetc(file);
		while(!feof(file)) {
			(frqs[ch])++;
			ch = fgetc(file);
		}
	}
	fclose(file);
	return true;
}

void write_frqs(long* frqs, const char* path) {
	FILE* fptr = fopen(path, "wb");
	if(fptr) {
		fwrite(frqs, sizeof(long), 256, fptr);
		fclose(fptr);
	}	
}