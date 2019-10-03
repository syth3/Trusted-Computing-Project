#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 


unsigned int size = 10;
uint8_t pubkey[10] = {0,1,2,3,4,5,6,7,8,9}; 
char *privkey = "sshpassword123";   
uint8_t array[256*4096];

#define CACHE_HIT_THRESHOLD (200)
#define DELTA 1024
#define STRLEN 8

// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
  //printf("%d\n", x);
  if (x < size) {
  	return pubkey[x];
  } else {
  	return 0;
  } 
}


void checkCache(int x){
  // Initialize the array
  int junk=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  
  for(i=0; i<10; i++) array[i*4096]=1;
  // FLUSH the array from the CPU cache
  for(i=0; i<10; i++) _mm_clflush(&array[i*4096]);
  // Access some of the array items
  int a = pubkey[x];
  array[a*4096] = 100;
  for(i=0; i<10; i++) {
    addr = &array[i*4096];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    printf("Access time for array[%d*4096]: %d CPU cycles\n",i, (int)time2);
  }
}

void checkCacheTime(){
  // Initialize the array
  int junk=0;
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  
  for(i=0; i<10; i++) array[i*4096]=1;
  // FLUSH the array from the CPU cache
  for(i=0; i<10; i++) _mm_clflush(&array[i*4096]);
  // Access some of the array items
  int a = pubkey[8];
  array[a*4096] = 100;
//  array[7*4096] = 200;
  for(i=0; i<10; i++) {
    addr = &array[i*4096];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    printf("Access time for array[%d*4096]: %d CPU cycles\n",i, (int)time2);
  }
}

// Function designed for chat between client and server. 
void func(int sockfd)
{
  char buff[MAX];
  int n;
  int m;
  // infinite loop for chat 
  for (;;) {
  	bzero(buff, MAX);

  	// read the message from client and copy it in buffer 
  	read(sockfd, buff, sizeof(buff));
  	// print buffer which contains the client contents 
  	sscanf(buff, "%d", &m);         
  	//printf("Client sent: %d\n", m); 
  	bzero(buff, MAX);
  	n = 0;

  	n = restrictedAccess(m);	
        //printf("Checking cachetime for %d\n", m);
        checkCache(m);
        //checkCacheTime();
  	sprintf(buff, "%d", n);
  	m = 0;

  	// and send that buffer to client 
  	write(sockfd, '\n', 1);
  	write(sockfd, buff, sizeof(buff));
  	write(sockfd, '\n', 1);

  	bzero(buff, MAX);

  }
}


int main() {
  printf("Memory address leaking exploit\n");
  printf("secret %p\n", privkey);
  printf("buffer %p\n", pubkey);
  printf("Differ %d\n", ((size_t)(privkey - (char*)pubkey)));
  int count = 0;
  int sockfd, connfd, len;
  struct sockaddr_in servaddr, cli;

  while(1){
    // socket create and verification 
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
  	printf("socket bind failed...\n");
  	exit(0);
    }
    else
  	printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) {
  	printf("Listen failed...\n");
  	exit(0);
    }

    else
  	printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
  	printf("server acccept failed...\n");
  	exit(0);
    }
    else
  	printf("server acccept the client...\n");
    func(connfd);
  }
  close(sockfd);
  return 0;
}
