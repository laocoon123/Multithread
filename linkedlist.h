// Priority queue
typedef struct Node{
	int id;
	int loadt;
	int crosst;
	pthread_mutex_t m;
	pthread_cond_t	c;
	struct Node* next;
}Node;

//return a head node
Node* newList(int data1, int data2, int data3, pthread_mutex_t mutex, pthread_cond_t convar){
	Node* newNode = malloc(sizeof(Node));

	newNode->id = data1;
	newNode->loadt = data2;
	newNode->crosst = data3;
	newNode->m = mutex;
	newNode->c = convar;
	newNode->next = NULL;
	return newNode;
}

//return a head node
Node* newNode(Node* head, int data1, int data2, int data3, pthread_mutex_t mutex, pthread_cond_t convar){
	if(head == NULL){
		head = newList(data1, data2, data3, mutex, convar);
	}else{
		Node* newNode = malloc(sizeof(Node));

		newNode->id = data1;
		newNode->loadt = data2;
		newNode->crosst = data3;
		newNode->m = mutex;
		newNode->c = convar;
		newNode->next = NULL;
		if(newNode->id < head->id){
			newNode->next = head;
			return newNode;
		}
		Node* curr = head;
		while(curr->next){
			if(curr->next->id > newNode->id){
				newNode->next = curr->next;
				curr->next = newNode;
				return head;
			}
			curr = curr->next;
		}

		curr->next = newNode;
	}

	return head;
}

// return the new head node
Node* pop(Node** head){
	Node* newHead;
	if(!(*head)->next){
		return NULL;
	}
	newHead = (*head)->next;
	free(*head);
	*head = newHead;
	
	return *head; 
}

void printNode(Node* curr){
	printf("id:%d, load:%d, cross:%d;\n", curr->id, curr->loadt, curr->crosst);
}

void printList(Node* head){
	Node* curr = head;
	if(head == NULL){
		printf("%s\n", "Empty list");
		return;
	}
	
	while(curr){
		printNode(curr);
		curr = curr->next;
	}
	printf("\n");
}

void freeList(Node* head){
	Node* curr = head;
	Node* next;
	while(curr != NULL){
		next = curr->next;
		free(curr);
		curr = next;
	}
}