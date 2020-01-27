#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <stdio.h>
#define SIZE 500
#define LENGTH 45

//hash table
typedef struct node{
	char *word;
	struct node *next;
}node;

static node *hashTable[SIZE];
//fuction to load dictionary into memory
int load (char* dictionary)
{
	int qu = 0;
	int word_count = 0;
	//open dictionary file
	FILE *file;
	file = fopen(dictionary,"r");

	if(file == NULL){
		printf("there was a problem opening the file");
		return 0;
	}

	char *dic_word;
	dic_word = calloc(LENGTH+1,sizeof(char));
	//scan until end of file
	while(fscanf(file,"%45s",dic_word)!= EOF){
		//creating new node
		node* new_node;


		//if malloc fails
		if((new_node = malloc(sizeof(node))) == NULL){
			return 0;
		}
		//if malloc fails
		if((new_node->word = strdup(dic_word)) == NULL){
			return 0;
		}
		
		strcpy(new_node->word,dic_word);

		unsigned int key = 0;

		//finding the location to store the word in the hash table
		for(int i =0;i<strlen(dic_word);i++){
			key = (key * dic_word[i] + dic_word[i] + i)%SIZE;
		}

			//putting it in the hash table
			new_node->next = hashTable[key];
			hashTable[key] = new_node;

			word_count++;
			qu = key;
		}

		printf("Word count - %d\n",word_count);
		fclose(file);
		return 1;
	}




