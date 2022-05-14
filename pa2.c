#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include "huffman.h"

int main(int argc, char ** argv) {
	if (argc != 7)
		return EXIT_FAILURE;
	int size = 0;
	long file_size;
	long coding_tree_size;
	long num_bytes_text;
	int location = 0;
	int byte_idx = 0;
	unsigned char* file_contents = read_file(argv[1], &size);
	if(!file_contents) {
		free(file_contents);
		return EXIT_FAILURE;
	} 
	
	// deal with the first 3 longs
	file_size = get_size(file_contents, 8, 0);
	coding_tree_size = get_size(file_contents, 8, 8);
	num_bytes_text = get_size(file_contents, 8, 16);
	
	// extract topology
	unsigned char* topology = malloc(sizeof(char) * coding_tree_size);
	for(int i = 0; i < coding_tree_size; i++) {
		topology[i] = file_contents[i + 24];
	}
	
	// write character-based representation of topology to file while also making a coding tree
	FILE *fptr = fopen(argv[2], "w");
	if(!fptr) {
		free(file_contents);
		free(topology);
		return EXIT_FAILURE;
	}
	TreeNode* top_tree = make_tree(topology, &location, &byte_idx, coding_tree_size, fptr);
	fclose(fptr);
	
	// extract encoded bytes
	unsigned char* decode_traversal = malloc(sizeof(char) * num_bytes_text);
	for(int i = 0; i < file_size - 24 - coding_tree_size; i++) {
		decode_traversal[i] = file_contents[i + 24 + coding_tree_size];
	}
	
	// decode the message from the coding tree
	location = 0;
	byte_idx = 0;
	
	fptr = fopen(argv[3], "w");
	if(!fptr) {
		free(file_contents);
		free(topology);
		free(decode_traversal);
		destroy_huff_tree(&top_tree);
		return EXIT_FAILURE;
	}
	FILE* fptr2 = fopen(argv[6], "w");
	if(!fptr2) {
		free(file_contents);
		free(topology);
		free(decode_traversal);
		destroy_huff_tree(&top_tree);
		fclose(fptr);
		return EXIT_FAILURE;
	}
	decode_tree(decode_traversal, location, byte_idx, top_tree, file_size - 24 - coding_tree_size, fptr, fptr2, num_bytes_text);
	fclose(fptr);
	
	// create frequency array for each character in the text
	long frqs[256] = {0};
	bool is_file = get_frqs(frqs, argv[3]);
	if(is_file)
		write_frqs(frqs, argv[4]);
	else {
		free(file_contents);
		free(topology);
		free(decode_traversal);
		destroy_huff_tree(&top_tree);
		fclose(fptr2);
		return EXIT_FAILURE;
	}
	
	// create huffman tree
	Queue* pqueue = make_tree_pq(frqs);
	TreeNode* huffman_tree = make_huff_tree(pqueue);
	
	// writing character based representation of the huffman tree to file
	fptr = fopen(argv[5], "w");
	if(!fptr) {
		free(file_contents);
		free(topology);
		free(decode_traversal);
		destroy_huff_tree(&top_tree);
		destroy_huff_tree(&huffman_tree);
		fclose(fptr2);
		return EXIT_FAILURE;
	}
	preorder_traverse(huffman_tree, fptr);
	fclose(fptr);
	
	// determine bits to huffman encode the message
	unsigned long huff_total = 0;
	huff_bits(huffman_tree, &huff_total, 0);
	unsigned long huff_byte = huff_total / 8;
	unsigned int huff_bits_remain = huff_total % 8;
	fwrite(&huff_byte, sizeof(long), 1, fptr2);
	fwrite(&huff_bits_remain, sizeof(int), 1, fptr2);
	fclose(fptr2);
	
	free(topology);
	free(decode_traversal);
	destroy_huff_tree(&huffman_tree);
	destroy_huff_tree(&top_tree);
	free(file_contents);
	return EXIT_SUCCESS;
}