#include <stdlib.h>
#include <string.h>
#include "cpen212alloc.h"

typedef struct {
    void *start;   // start of heap
    void *end;  // end of heap
} alloc_state_t;

//----------------------------------------------------//
// This is an implicit Free list. Each 'block' contains a Header,
// Footer, and Data Space. It uses a First Fit Policy.
// The Header and Footer for a block are identical
// They are 8 byte unsigned long ints.
// The first bit is the Header/Footer is  the allocated t/f bit, 
// The rest is the size of the block including the Header and footer
// ---------------------------------------------------------------// 


// good
static void *findNextApplicable(void* heapEnd, void *startaddress, size_t nbytes){

    if (startaddress + 16 + nbytes >= heapEnd){
        return NULL;
    }
    unsigned long int currentHeader = *(unsigned long int*) startaddress;
    unsigned long int Status = currentHeader &1;
    size_t sizeOfCurrentBlock = currentHeader >> 1;

   if (Status == 0 && sizeOfCurrentBlock >= (nbytes + 16)){
        return startaddress;
    }

    else {
        void * nextAddressToLookAt = startaddress + sizeOfCurrentBlock;
        return findNextApplicable(heapEnd, nextAddressToLookAt, nbytes);
    }

}

// good
static void cpen212_coalesceWithPrev(void *alloc_state, void *addressOfHeader){
    alloc_state_t *allocator = (alloc_state_t *) alloc_state;


    // if nothing before it, dont do anything
    if (addressOfHeader == allocator->start){
        return;
    }
    
    void* addressOfPrevFooter = (addressOfHeader - 8);

    unsigned long int prevFooterValue = *(unsigned long int *) addressOfPrevFooter;
    unsigned long int Status = prevFooterValue & 1;

    if(Status == 1){
        return;
    }
    else {
        size_t sizeOfPrevBlock = prevFooterValue >> 1;

        unsigned long int currentHeaderValue = *(unsigned long int *) addressOfHeader;
        size_t sizeOfCurrentBlock = currentHeaderValue >> 1;

        size_t newBlockSize = sizeOfCurrentBlock + sizeOfPrevBlock;

        void *addressOfFooter = addressOfHeader + (sizeOfCurrentBlock - 8);

        void* addressOfPrevHeader = (addressOfHeader - sizeOfPrevBlock);

        unsigned long int fullNewHeaderFooter = newBlockSize << 1;

        fullNewHeaderFooter &= ~1;

        *(unsigned long int*) addressOfPrevHeader = fullNewHeaderFooter;
        *(unsigned long int*) addressOfFooter = fullNewHeaderFooter;   

        return;
    }
}

static void cpen212_coalesceWithNext(void *alloc_state, void *addressOfFooter){

 alloc_state_t *allocator = (alloc_state_t *) alloc_state;

    // if nothing after it, dont do anything
    if (addressOfFooter == allocator->end -8){
        return;
    }


    void* addressOfNextHeader = (addressOfFooter + 8);

    unsigned long int nextHeaderValue = *(unsigned long int *) addressOfNextHeader;
    unsigned long int Status = nextHeaderValue & 1;

    if(Status == 1){
        return;
    }
    else {
        size_t sizeOfNextBlock = nextHeaderValue >> 1;

        unsigned long int currentFooterValue = *(unsigned long int *) addressOfFooter;
        size_t sizeOfCurrentBlock = currentFooterValue >> 1;

        size_t newBlockSize = sizeOfCurrentBlock + sizeOfNextBlock;

        void *addressOfHeader = addressOfFooter - sizeOfCurrentBlock + 8;

        void* addressOfNextFooter = (addressOfFooter + sizeOfNextBlock);

        unsigned long int fullNewHeaderFooter = newBlockSize << 1;

        fullNewHeaderFooter &= ~1;

        *(unsigned long int*) addressOfHeader = fullNewHeaderFooter;
        *(unsigned long int*) addressOfNextFooter = fullNewHeaderFooter;   
    }


}


// good
void *cpen212_init(void *heap_start, void *heap_end) {
    alloc_state_t *s = (alloc_state_t *) malloc(sizeof(alloc_state_t));

    
    // heap rounded down to the multiple of 16 bytes
    size_t size = heap_end - heap_start; 
    size_t alignedHeapSize = size & ~15;

    s->start = heap_start;
    s->end = (heap_start + alignedHeapSize);

    unsigned long int headerAndFooter = alignedHeapSize << 1; // LSL by one so we can use LSB as t/f
    headerAndFooter &= ~1; // sets the LSB as 0

    // headerPointer points to start of heap and header/footer is placed there
    *(unsigned long  int*) heap_start = headerAndFooter;
                    //unsigned long int *startOfHeader = heap_start;
                    //unsigned long int *endOfHeap = heap_end;
                    // footerPointer points to end of heap - 8 bytes and header/footer is placed there
    unsigned long int *startOfFooter = (unsigned long int*) (s->end-8);
    *startOfFooter = headerAndFooter;

    return s;
}

// good
void cpen212_deinit(void *s) {
    // free all memory allocated 
    // delete s->start;
    // delete s->end;
    free(s);
}

// good
void *cpen212_alloc(void *alloc_state, size_t nbytes) {
    if (nbytes == 0) return NULL;
    alloc_state_t *allocator = (alloc_state_t *) alloc_state;

    size_t aligned_sz = (nbytes + 7) & ~7;
    
    size_t totalSizeOfAllocatedMem = aligned_sz + 16; // size needed plus room for header/footer

    // if null means no space, else returns pointer start of header of available space 
    // takes size needed not including header or footer size
    void *currentHeader = findNextApplicable(allocator->end, allocator->start, aligned_sz);
    if (currentHeader == NULL){
        return NULL;
    }
    

    size_t sizeOfAvailableBlock = *(unsigned long int*) currentHeader >> 1;

    // if exact fit, no need to make a new header, 
    if (sizeOfAvailableBlock == totalSizeOfAllocatedMem){
        // set as taken
        *(unsigned long int*) currentHeader |= 1;

        unsigned long int* footerAddress = (unsigned long int* )(currentHeader + 8 + aligned_sz);
        *footerAddress = *(unsigned long int*) currentHeader;
        void *userInputAddress = (currentHeader + 8);
        return userInputAddress;
    }

    // else we need to make a new header and everything
    else {

        // size of next free block is old size -  totalSizeOfAllocatedMem
        
        size_t totalFreeSize = sizeOfAvailableBlock - totalSizeOfAllocatedMem; 

        unsigned long int newFreeHeaderFooter = totalFreeSize << 1;
        newFreeHeaderFooter &= ~1; // to make sure its set as free
        // value at newHeader is newHeaderFooter


        // sets new freeHeader/Footer at addresss of new free header
        unsigned long int* addressOfNewFreeHeader = (unsigned long int*) (currentHeader + totalSizeOfAllocatedMem);
        *addressOfNewFreeHeader = newFreeHeaderFooter;

        // sets new freeHeader/Footer at addresss of new free header
        unsigned long int* addressOfNewFreeFooter = (unsigned long int*) (currentHeader + sizeOfAvailableBlock -8);
        *addressOfNewFreeFooter = newFreeHeaderFooter;
      

        // head/footer for current block that is about to be allocated
         unsigned long int newHeaderFooter = totalSizeOfAllocatedMem << 1;
        // sets as allocated
        newHeaderFooter |= 1;

        *(unsigned long int*) currentHeader  = newHeaderFooter;
        // footer of to be allocated at memAddrss header + aligned_size + 8
        unsigned long int *footerAddress = (unsigned long int *) (currentHeader + aligned_sz + 8);
        *footerAddress = newHeaderFooter;

        return (currentHeader + 8);

    }
}

// good
void cpen212_free(void *s, void *p) {
    
    // the first bit of the header is set to 0 to indicate not allocated
    unsigned long int *addressOfHeader = (unsigned long int*) (p-8);
    
    size_t sizeOfBlock = *addressOfHeader >> 1;

    // set header as unallocated
    *addressOfHeader &= ~1;

    // p + size -16 is footer address
    unsigned long int *addressOfFooter = (unsigned long int*) (p + sizeOfBlock - 16);

    *addressOfFooter &= ~1; // the footer is set to unallocated.
   
    cpen212_coalesceWithPrev(s, addressOfHeader);
    cpen212_coalesceWithNext(s, addressOfFooter);

    return;
 }

//good
void *cpen212_realloc(void *s, void *prev, size_t nbytes) {
    if (prev == NULL){
        return cpen212_alloc(s, nbytes);
    }
 //  alloc_state_t *allocator = (alloc_state_t *) s;    

    size_t alignedSizeOfNewMem = (nbytes + 7) & ~7;
   // size_t totalSizeOfNewMem = alignedSizeOfNewMem + 16;

    unsigned long int* oldHeaderAddr = (unsigned long int*) (prev - 8);
    
    size_t totalSizeOfPrevBlock = *oldHeaderAddr >> 1;

    size_t actualPrevAllocatedSpace = totalSizeOfPrevBlock - 16;

    // if the memory wanted is the same as they have, return prev
    if(actualPrevAllocatedSpace == alignedSizeOfNewMem){
        return prev;
    }

    // allocate new memory 
    void * newWriteableMem = cpen212_alloc(s, nbytes);
    if (newWriteableMem == NULL) {
            return NULL;
    }
    /////////////////// might need to change to alignedsize instead of nbytes
    if (actualPrevAllocatedSpace > nbytes){
        memmove(newWriteableMem, prev, nbytes);
    }
    else{
        memmove(newWriteableMem, prev, actualPrevAllocatedSpace);
    }

    // free prvious memory 
    cpen212_free(s, prev);

    return newWriteableMem;
}


bool cpen212_check_consistency(void *alloc_state) {
    // alloc_state_t *s = (alloc_state_t *) alloc_state;
    // return s->end > s->free;
    return true;
}
