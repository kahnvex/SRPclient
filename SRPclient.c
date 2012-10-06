/* client.c */
/* This is a sample UDP client/sender.  */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   
#include <sys/time.h> 
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "types.h"


#define REMOTE_SERVER_PORT 50005
#define SWS 4
#define MAXSEQNUM 8
#define FILESIZE (24*1024)
#define BUFSIZE 100
#define DELAY 1
//#define DELM "/"

struct sockaddr_in cliAddr, remoteServAddr, recAddr;
const FILE* fp;
int sock;

int main(int argc, char *argv[]) {

  static int selret;

  unsigned int recLen;
  static char* p;
  static char filebuffer[FILESIZE];
  static char directemp[BUFSIZE];
  static char filename[BUFSIZE];
  static struct timeval time;

  static fd_set readFDS;
  static frame frameArray[MAXSEQNUM];
  static int LB = 0, RB = 0; // static
  static char ackBuffer[100];
  static ack recvAck;
  static timeStruct timesArray[SWS];
  int lastSeqNum = -1;
  double t, timeout;
  int lastFrameCount = 0;

  //check command line args.
  if(argc < 6) {
    printf("usage : %s <server> <error rate> <random seed> <send_file> <send_log> \n", argv[0]);
    exit(1);
  }

  printf("argv4: %s\n",argv[4]);

  if(strcmp(argv[4],"0")){

    strcpy(directemp,argv[4]);
    p = strtok(directemp,DELM);
      
    while(p != NULL){
      strcpy(filename,p);
      p= strtok(NULL,DELM);
    }
      
    printf("Filename: %s\n",filename);
    fp = fopen(argv[4],"r");
    if(fp == NULL){
      printf("Error opening file: %s\n",strerror(errno));
    }
     
    //strcpy(msgbuffer,filebuffer);
  }

  
  
  printf("error rate : %f\n",atof(argv[2]));
  printf("%s: sending data to '%s' \n", argv[0], argv[1]);
    

  /* Note: you must initialize the network library first before calling
     sendto_().  The arguments are the <errorrate> and <random seed> */
  init_net_lib(atof(argv[2]), atoi(argv[3]));

  /* get server IP address (input must be IP address, not DNS name) */


  //setup socket structs
  /***************************************************/
  remoteServAddr.sin_family = AF_INET;
  remoteServAddr.sin_port = htons(REMOTE_SERVER_PORT);
  inet_pton(AF_INET, argv[1], &remoteServAddr.sin_addr);
  
  cliAddr.sin_family = AF_INET;
  cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliAddr.sin_port = htons(0);
  /*************************************************/



  /* socket creation */
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("Unable to connect socket!\n");
    exit(1);
  }
  int lastFrameSent = 0;
  while(1){
	  bzero(&ackBuffer, sizeof(ackBuffer));
    
	  int moveCount = MoveForward(&LB, &RB, frameArray, MAXSEQNUM);
	  if(lastFrameSent == 0){
		  lastFrameSent = SendNextFrames(moveCount, frameArray, MAXSEQNUM, LB, &fp,
				  	  	  &sock,(struct sockaddr *) &remoteServAddr, timesArray, SWS);
	  }

/*
    if((sendto_(sock,sendFrame, strlen(sendFrame),0, (struct sockaddr *) &remoteServAddr,
		sizeof(remoteServAddr)))< 0 ){
      printf("Error Sending\n");
      perror("sendto()");
      exit(1);
    }*/

	  gettimeofday(&time,NULL);
	  t = (time.tv_sec + (time.tv_usec/1000000.0));


	  timeout = (currentDeadline(timesArray,SWS) + DELAY) - t;
	  printf("Timeout: %f\n", timeout);
	  if(timeout < 0){timeout = 0;}

    selret = ballinselect(sock,&readFDS,timeout,0);

    
    if ((selret != -1) && (selret != 0)){
      
      if(recvfrom(sock, &ackBuffer, sizeof (ackBuffer), 0,(struct sockaddr *) &recAddr, (socklen_t*) &recLen) < 0){
    	  perror("recvfrom()");
    	  exit(1);
      }

      printf("Ack Buffer: %s\n",ackBuffer);

      makeackstruct(ackBuffer, &recvAck);
      if(lastSeqNum != recvAck.seqNum){
    	  frameArray[recvAck.seqNum].ack = 1;
    	  lastSeqNum = recvAck.seqNum;

    	  printf("============================\n");
    	  printf("Received Ack!\n");
    	  printf("Ack SeqNum: %d\n", recvAck.seqNum);
    	  printf("============================\n\n\n");

    	  removefromtimearray(recvAck.seqNum, timesArray,SWS);
      }

    }
    else if (selret == 0) {
      printf("timeout\n");
      bzero(&filebuffer,sizeof(filebuffer));

      int timeoutframe = FindTimeout(timesArray,SWS);



      if(frameArray[timeoutframe].lastFrame == 1){
    	  if(frameArray[timeoutframe].ack == 1){return EXIT_SUCCESS;}
    	  if(lastFrameCount == 10){return EXIT_SUCCESS;}
    	  lastFrameCount ++;
      }

      MakePacket(filebuffer,frameArray[timeoutframe]);

      printf("\nSending Timed out Frame: \n\n");
      printFrame(frameArray[timeoutframe]);

      if((sendto_(sock,filebuffer, strlen(filebuffer),0, (struct sockaddr *) &remoteServAddr,
      		sizeof(remoteServAddr)))< 0 ){
            printf("Error Sending\n");
            perror("sendto()");
            exit(1);
      }

      /*gettimeofday(&time,NULL);
      double t2= time.tv_sec + (time.tv_usec/1000000.0);
      printf("Time Out Time time: %f\n",t2);*/
    } else if (selret < 0) {printf("select() failed\n");}
  }

  fclose(fp);
    
  return EXIT_SUCCESS;
}
