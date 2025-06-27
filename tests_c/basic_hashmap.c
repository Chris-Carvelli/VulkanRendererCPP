#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <cc_allocator.h>

const int CAPACITY = 4096;

// Linked List node
struct node {

    // key is string
    char* key;

    // value is also string
    char* value;
    struct node* next;
};

// like constructor
void setNode(struct node* node, char* key, char* value)
{
    node->key = key;
    node->value = value;
    node->next = NULL;
    return;
};

struct hashMap {

    // Current number of elements in hashMap
    // and capacity of hashMap
    int numOfElements, capacity;

    // hold base address array of linked list
    struct node** arr;
};

// like constructor
void initializeHashMap(struct hashMap* mp)
{

    // Default capacity in this case
    mp->capacity = CAPACITY;
    mp->numOfElements = 0;

    // array of size = 1
    mp->arr = (struct node**)malloc(sizeof(struct node*)
        * mp->capacity);
    memset(mp->arr, 0, sizeof(struct node*)
        * mp->capacity);
    return;
}

int hashFunction(struct hashMap* mp, char* key)
{
    int bucketIndex;
    int sum = 0, factor = 31;
    for (int i = 0; i < strlen(key); i++) {

        // sum = sum + (ascii value of
        // char * (primeNumber ^ x))...
        // where x = 1, 2, 3....n
        sum = ((sum % mp->capacity)
            + (((int)key[i]) * factor) % mp->capacity)
            % mp->capacity;

        // factor = factor * prime
        // number....(prime
        // number) ^ x
        factor = ((factor % __INT16_MAX__)
            * (31 % __INT16_MAX__))
            % __INT16_MAX__;
    }

    bucketIndex = sum;

    if (bucketIndex >= mp->capacity)
        printf("WTF?");
    return bucketIndex;
}

void insert(struct hashMap* mp, char* key, char* value)
{

    // Getting bucket index for the given
    // key - value pair
    int bucketIndex = hashFunction(mp, key);
    struct node* newNode = (struct node*)malloc(

        // Creating a new node
        sizeof(struct node));

    // Setting value of node
    setNode(newNode, key, value);

    // Bucket index is empty....no collision
    if (mp->arr[bucketIndex] == NULL) {
        mp->arr[bucketIndex] = newNode;
    }

    // Collision
    else {

        // Adding newNode at the head of
        // linked list which is present
        // at bucket index....insertion at
        // head in linked list
        newNode->next = mp->arr[bucketIndex];
        mp->arr[bucketIndex] = newNode;
    }
    return;
}

void delete (struct hashMap* mp, char* key)
{

    // Getting bucket index for the
    // given key
    int bucketIndex = hashFunction(mp, key);

    struct node* prevNode = NULL;

    // Points to the head of
    // linked list present at
    // bucket index
    struct node* currNode = mp->arr[bucketIndex];

    while (currNode != NULL) {

        // Key is matched at delete this
        // node from linked list
        if (strcmp(key, currNode->key) == 0) {

            // Head node
            // deletion
            if (currNode == mp->arr[bucketIndex]) {
                mp->arr[bucketIndex] = currNode->next;
            }

            // Last node or middle node
            else {
                prevNode->next = currNode->next;
            }
            free(currNode);
            break;
        }
        prevNode = currNode;
        currNode = currNode->next;
    }
    return;
}

char* search(struct hashMap* mp, char* key)
{

    // Getting the bucket index
    // for the given key
    int bucketIndex = hashFunction(mp, key);

    // Head of the linked list
    // present at bucket index
    struct node* bucketHead = mp->arr[bucketIndex];
    while (bucketHead != NULL) {

        // Key is found in the hashMap
        if (bucketHead->key == key) {
            return bucketHead->value;
        }
        bucketHead = bucketHead->next;
    }

    // If no key found in the hashMap
    // equal to the given key
    char* errorMssg = (char*)malloc(sizeof(char) * 25);
    errorMssg = "Oops! No data found.\n";
    return errorMssg;
}

void hist(struct hashMap* mp) {
    uint32_t buckets[mp->capacity];
    memset(buckets, 0, sizeof(buckets));

    int num_keys = 0;
    int num_hops = 0;

    for(int i = 0; i < mp->capacity; ++i) {
        if(mp->arr[i] == NULL) {
            buckets[0]++;
            continue;
        }

        int chain_len = 0;
        num_hops++;
        struct node* bucketHead = mp->arr[i];
        while (bucketHead != NULL) {
            bucketHead = bucketHead->next;
            num_hops++;
            ++chain_len;
        }
        buckets[chain_len]++;
        num_keys += chain_len;
    }


    printf("len\tCount\tHops\n");
    printf("%3d\t%5d\t%5d\n", 0, buckets[0], 0);
    for(int i = 1; i < mp->capacity; ++i) {
        if (buckets[i] == 0)
            continue;
        printf("%3d\t%5d\t%5d\n", i, buckets[i], buckets[i] * i);
    }
    printf("num keys:\t%3d\tnum hops:\t%3d\tfract:\t%f\n", num_keys, num_hops, (float)num_hops / (float)num_keys);
    printf("empty bins: %d\t%f\n", buckets[0], (float)buckets[0]/(float)mp->capacity);
}

// Drivers code
int main()
{

    // Initialize the value of mp
    struct hashMap* mp
        = (struct hashMap*)malloc(sizeof(struct hashMap));
    initializeHashMap(mp);

    const int NUM_ELEMENTS = CAPACITY;
    BumpAllocator* a = allocator_make_bump(KB(64));
    char* keys[NUM_ELEMENTS];
    for(int i = 0; i < NUM_ELEMENTS; ++i) {
        char* key   = (char*)allocator_alloc(a, sizeof(uint32_t));
        char* value = (char*)allocator_alloc(a, sizeof(uint32_t));
        _itoa(i, key, 10);
        _itoa(i, value, 10);
        insert(mp, key, value);
        keys[i] = key;
    }

    hist(mp);

    return 0;
}