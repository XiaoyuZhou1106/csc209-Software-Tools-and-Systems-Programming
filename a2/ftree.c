#include <stdio.h>
// Add your system includes here.
#include <stdlib.h>
#include "ftree.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

struct dirent *get_first_entry_ptr(char *fname, char *path) {
    DIR *d_ptr = opendir(path); 
    if (d_ptr == NULL) {
	return NULL;
    }
	
    struct dirent *entry_ptr;
    entry_ptr = readdir(d_ptr);

    while (entry_ptr != NULL && (entry_ptr->d_name)[0] == '.') {
	    entry_ptr = readdir(d_ptr);
    }
    return entry_ptr;
}

struct dirent *get_other_entry_ptr(char *fname, char *path) {
    DIR *d_ptr = opendir(path); 
    if (d_ptr == NULL) {
	return NULL;
    }
	
    struct dirent *entry_ptr;
    entry_ptr = readdir(d_ptr);
    int read_time = 0;

    while (entry_ptr != NULL && (read_time == 0 || (entry_ptr->d_name)[0] == '.')){
	    read_time = (strcmp(entry_ptr->d_name, fname) == 0) ? 1 : 0; 
	    entry_ptr = readdir(d_ptr);
    }
    return entry_ptr;
}

struct TreeNode *construct_tree(char *fname, char *path) {
    char new_path[strlen(fname) + strlen(path) +2];
    memset(new_path, '\0', sizeof(new_path));

    strncpy(new_path, path, strlen(path));
    strncat(new_path, "/", 1);
    strncat(new_path, fname, strlen(fname)+1);
  
    struct TreeNode *node;
    node = malloc(sizeof(struct TreeNode));

    struct stat stat_buf;     
    if (lstat(new_path, &stat_buf) == -1) {
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
        return NULL;
    }

    char *file_name = malloc(sizeof(char) * (strlen(fname)+1));
    strcpy(file_name, fname);
    node->fname = file_name;
    node->permissions = stat_buf.st_mode & 0777;
    node->next = NULL; 
  
    if (S_ISREG(stat_buf.st_mode)) {
        node->type = '-';
	     node->contents = NULL;
    }else if (S_ISDIR(stat_buf.st_mode)) {
        node->type = 'd';
        
        if (get_first_entry_ptr(fname, new_path) != NULL) {
       	 	node->contents = construct_tree(get_first_entry_ptr(fname, new_path)->d_name, new_path);
        }else {
		   	node->contents = NULL;
		   	return node;
		  }
        struct TreeNode *current_node = node->contents;
        while (get_other_entry_ptr(current_node->fname, new_path) != NULL) {
            struct TreeNode *next_node; 
            next_node = construct_tree(get_other_entry_ptr(current_node->fname, new_path)->d_name, new_path);
            current_node->next = next_node;
            current_node = next_node;
        }
    }else {
        node->type = 'l';
        node->contents = NULL;
    }

    return node;
}

/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {

    // Your implementation here.

    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the 
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.
    struct TreeNode *node;
    struct stat stat_buf;

    if (lstat(fname, &stat_buf) == -1) {
        fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
        return NULL;
    }
	 
    char *new_name = malloc((strlen(fname) + 1) * sizeof(char));
    strncpy(new_name, fname, strlen(fname));
    new_name[-1] = '\0';
    
    node = construct_tree(new_name, ".");
	 
    free(new_name);

    return node;
}


void print_ftree_helper(struct TreeNode *root, int depth) {
	
    
    printf("%*s", depth * 2, "");

    // Your implementation here.
    struct TreeNode *child = root->contents;
    if (root->type == 'd') {
        printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
	     if (child != NULL) {
            print_ftree_helper(child, ++depth);
            struct TreeNode *child_next;
            child_next = child->next;
            while (child_next != NULL) {
                print_ftree_helper(child_next, depth);
                child_next = child_next->next;
            }
				depth--;
	}
    }else {
        printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
    }
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {
	
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    printf("%*s", depth * 2, "");

    // Your implementation here.
    print_ftree_helper(root, depth);
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
   // Your implementation here.
   struct TreeNode *child = node->contents;
    if (child != NULL) {
        deallocate_ftree(child);
    }
    struct TreeNode *next = node->next;
    if (next != NULL) {
        deallocate_ftree(next);
    }
    free(node);
}
