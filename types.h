#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   /* memset() */
#include <sys/time.h> /* select() */
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "sendto_.h"
#include <sys/socket.h>


#define INTSIZE 4
#define DELM "/"

typedef struct{
	double time;
	int seqNum;
	int isValid;
} timeStruct;

typedef struct{

  int seqNum;
  int lastFrame; //Last frame flag
  int dataSize;
  int ack;
  //char filename[100];
  char data[512];
} frame;

typedef struct{

  int seqNum;
}ack;

void removefromtimearray(int seqNum, timeStruct timearray[], int size){
	int i;
	for(i = 0; i < size; i++){
		if(seqNum == timearray[i].seqNum && timearray[i].isValid == 1){
			timearray[i].isValid = 0;
			return;
		}

	}
	printf("Remove from time array error!\n");
	exit(1);
}

int findtimeout(timeStruct timearray[], int size){
	int i;
	int smallesttime;
	int small = 0;
	struct timeval time;

	smallesttime = timearray[0].time;

	for(i = 0; i < size; i++){
		if(timearray[i].time < smallesttime && timearray[i].isValid == 1){
			smallesttime = timearray[i].time;
			small = i;
		}

	}

	gettimeofday(&time,NULL);
	double t = time.tv_sec + (time.tv_usec/1000000.0);

	timearray[i].time = t;

	return small;


}

int currentDeadline(timeStruct timearray[], int size){
	int i;
	int smallesttime;

	smallesttime = timearray[0].time;

	for(i = 0; i < size; i++){
		if(timearray[i].time < smallesttime && timearray[i].isValid == 1){
			smallesttime = timearray[i].time;
		}

	}

	return smallesttime;

}


int MoveForward(int* LB, int* RB, frame frameArray[], int arraySize) {
  int moveCount = 0, i;
  
  /* Special case: initialization: */
  if(*LB == *RB) {

	  *LB = 0;
	  *RB = 4;
	  moveCount = 4;
	  return moveCount;

  }

  /* Normal cases : */
  if(*LB < *RB){
    
    for(i = *LB; i < *RB && frameArray[i].ack == 1; i++) {
      moveCount++;
      frameArray[i].ack = 0;
    }
    
  } else if (*LB > *RB) {
    
    for(i = *LB; i < arraySize && frameArray[i].ack == 1; i++) {
      moveCount++;
      frameArray[i].ack = 0;
    }
    
    for(i = 0; i < *RB && frameArray[i].ack == 1; i++) {
      moveCount++;
      frameArray[i].ack = 0;
    }

  }

  /* Set right bound and leftbount according to how far
   * everything moved 
   */
  *LB = ( *LB + moveCount ) % arraySize;
  *RB = ( *RB + moveCount ) % arraySize;
  
  return moveCount;
}

void InitFrames(frame frameArray[], int arraySize) {
	int i;
	for(i = 0; i < arraySize; i++) frameArray[i].ack = 1;
}

void setFrame(frame* f, int seqnum, int lframe, int dsize, int ack, char* data){

  if(sizeof(data) > 512){
    printf("setFrame data packet is too large.\n");
    exit(1);

  }

  (*f).seqNum = seqnum;
  (*f).lastFrame = lframe;
  (*f).dataSize = dsize;
  (*f).ack = ack;
  //strcpy((*f).filename,fname);
  strcpy((*f).data,data);
}


int readtoframe(char* c, const FILE** fp){
	printf("This should show up, or else we are fucked");
	int result = 0;

  int readResult = fread(c,1,512,*fp);

  if(readResult < 512){result = 1;}

  return result;

}

int SendNextFrames( int moveCount, frame frameArray[], int arraySize, int LB, const FILE** fp, int* sock,
					 struct sockaddr * remoteServAddr, timeStruct timesArray[], int timeSize )
{
  int i, j, startSeq, finishSeq, isLastFrame;
  char data[513];
  char sendBuffer[24 * 1024];
  struct timeval time;

  startSeq = ( LB + 4 - moveCount ) % arraySize;
  finishSeq = ( startSeq + moveCount ) % arraySize;
  printf("============================\n");
  printf("startSeq = %d\n", startSeq);
  printf("finishSeq = %d\n", finishSeq);
  printf("moveCount = %d\n", moveCount);
  printf("============================\n\n\n");

  if(startSeq < finishSeq){
    
    for(i = startSeq; i < finishSeq; i++) {
    	bzero(&sendBuffer, sizeof(sendBuffer));
    	bzero(&data, sizeof(data));
    	printf("Got to here1\n");
    	isLastFrame = readtoframe(data, fp);
    	setFrame(&frameArray[i], i, isLastFrame, strlen(data), 0, data);
    	makedatapacket(sendBuffer, frameArray[i]);
    	printFrame(frameArray[i]);

    	/* Time shit */
    	gettimeofday(&time,NULL);
    	double t = time.tv_sec + (time.tv_usec/1000000.0);
    	printf("Sent time: %f\n", t);
    	for(j = 0; j < timeSize; j++){
    		if(timesArray[j].isValid == 0){
    			timesArray[j].isValid = 1;
    			timesArray[j].seqNum = i;
    			timesArray[i].time = t;
    			break;
    		}
    	}

    	/* Send it */
    	if((sendto_(*sock,sendBuffer, strlen(sendBuffer),0, remoteServAddr, sizeof(*remoteServAddr)))< 0 ){
    		printf("Error Sending\n");
    		perror("sendto()");
    		exit(1);
    	}

    	if(frameArray[i].lastFrame == 1){return 1;}


    }
    
  } else if (startSeq > finishSeq) {
    
    for(i = startSeq; i < arraySize; i++){
    	bzero(&sendBuffer, sizeof(sendBuffer));
    	printf("Got to here2\n");
    	isLastFrame = readtoframe(data, fp);
    	setFrame(&frameArray[i], i, isLastFrame, strlen(data), 0, data);
    	makedatapacket(sendBuffer, frameArray[i]);
    	printFrame(frameArray[i]);

    	/* Time shit */
		gettimeofday(&time,NULL);
		double t = time.tv_sec + (time.tv_usec/1000000.0);
		printf("Sent time: %f\n", t);
		for(j = 0; j < timeSize; j++){
			if(timesArray[j].isValid == 0){
				timesArray[j].isValid = 1;
				timesArray[j].seqNum = i;
				timesArray[i].time = t;
				break;
			}
		}

    	if((sendto_(*sock,sendBuffer, strlen(sendBuffer),0, remoteServAddr, sizeof(*remoteServAddr)))< 0 ){
    		printf("Error Sending\n");
    		perror("sendto()");
    		exit(1);
       }

    	if(frameArray[i].lastFrame == 1){return 1;}
    }

    for(i = 0; i < finishSeq; i++) {
    	bzero(&sendBuffer, sizeof(sendBuffer));
    	printf("Got to here3\n");
    	isLastFrame = readtoframe(data, fp);
    	setFrame(&frameArray[i], i, isLastFrame, strlen(data), 0, data);
    	makedatapacket(sendBuffer, frameArray[i]);
    	printFrame(frameArray[i]);

    	/* Time shit */
		gettimeofday(&time,NULL);
		double t = time.tv_sec + (time.tv_usec/1000000.0);
		printf("Sent time: %f\n", t);
		for(j = 0; j < timeSize; j++){
			if(timesArray[j].isValid == 0){
				timesArray[j].isValid = 1;
				timesArray[j].seqNum = i;
				timesArray[i].time = t;
				break;
			}
		}

    	if((sendto_(*sock,sendBuffer, strlen(sendBuffer),0, remoteServAddr, sizeof(*remoteServAddr)))< 0 ){
    		printf("Error Sending\n");
    		perror("sendto()");
    		exit(1);
    	}

    	if(frameArray[i].lastFrame == 1){return 1;}
    }
    
  }
  return 0;
}

int ballinselect(int sock, fd_set* readFDS, int tsec, int tusec){
  
  
  int iSockRet, iSelRet;
  struct timeval timeVal;

  timeVal.tv_sec = tsec;
  timeVal.tv_usec = tusec;
  
  FD_ZERO(readFDS);
  FD_SET(sock, readFDS);

  iSelRet = select(FD_SETSIZE, readFDS, NULL, NULL, &timeVal);

  return iSelRet;

}

void setAck(ack* a, int seqnum){
  (*a).seqNum = seqnum;

}

void printFrame(frame f){

  printf("======================================\n");
  printf("Frame Sequence Number: %d\n",f.seqNum);
  printf("Last Frame?: %d\n",f.lastFrame);
  printf("Frame Size: %d\n",f.dataSize);
  //printf("File name: %s\n",f.filename);
  printf("Frame ack?: %d\n",f.ack);
  printf("Data: %s\n",f.data);
  printf("======================================\n");
}


void makeackmsg(char*acknowledge, ack* a){

  sprintf(acknowledge,"%d",(*a).seqNum);

}

void makeackstruct(char* a, ack* ackreturn){


  (*ackreturn).seqNum = atoi(a);
}

void makeackfromframe(ack* ackreturn,frame* f){

  (*ackreturn).seqNum = (*f).seqNum;

}


void makedatapacket(char* creturn, frame f){
  

  char sNum[4];
  char finish[4];
  char dSize[4];
  char ack[4];
  char data[512];
  char delm[] = DELM;

  sprintf(finish,"%d",f.lastFrame);
  sprintf(dSize,"%d",f.dataSize);
  sprintf(ack,"%d",f.ack);
  sprintf(data,"%s",f.data);
  sprintf(sNum,"%d",f.seqNum);

  /* Concat together all fields and add delims */
  strcat(creturn,sNum);
  strcat(creturn,delm);
  strcat(creturn,finish);
  strcat(creturn,delm);                                                                  
  strcat(creturn,dSize);
  strcat(creturn,delm);
  strcat(creturn,ack);
  strcat(creturn,delm);
  strcat(creturn,data);

}

void makedatastruct(char* c, frame* sreturn){

  //frame* sreturn = calloc(600,1);
  char* p;
  char sNum[INTSIZE];
  char finish[INTSIZE];
  char dSize[INTSIZE];
  char ack[INTSIZE];
  //char fname[100];
  char data[512];
  
  p = strtok(c,DELM);
  strcpy(sNum,p);
  p = strtok(NULL,DELM);
  strcpy(finish,p);
  p = strtok(NULL,DELM);
  strcpy(dSize,p);
  p = strtok(NULL,DELM);
  strcpy(ack,p);
  p = strtok(NULL,DELM);
  strcpy(data,p);
  
  (*sreturn).seqNum = atoi(sNum);
  (*sreturn).lastFrame = atoi(finish);
  (*sreturn).dataSize = atoi(dSize);
  (*sreturn).ack = atoi(ack);
  //strcpy((*sreturn).filename,fname);
  strcpy((*sreturn).data,data);
  
  //return sreturn;
}
