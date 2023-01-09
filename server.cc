#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <pthread.h>	//thread creation library
#include "inventory.h"
#include "order.h"
#include <string>
#include <list>
using namespace std;

inventory* regularInventory = new inventory(false); 	//false actual, allocated pass true
inventory* allocInventory = new inventory(true); // pass true

//Create Mutex List for Regular Inventory
pthread_mutex_t* regularInventoryMutexes;

//Create Mutex List for Allocated Inventory
pthread_mutex_t* allocInventoryMutexes;


void mutexInit(pthread_mutex_t** mutexArray){
	int arrayLength = regularInventory->inv.size();

	*mutexArray = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*arrayLength);
	for(int i = 0; i < arrayLength; i++){
		pthread_mutex_init(*mutexArray + i, NULL);
	}
}

void printStock() {

	std::stringstream buffer;

	//Print Inventory Stock
	buffer << "INVENTORY\n";
	for (int i=0; i<regularInventory->inv.size();i++){
		buffer<< regularInventory->inv[i]->amount <<'\n';
	}
	buffer<< "ALLOCATED INVENTORY\n";
	//Print ALlocate Stock
	for (int i=0; i<allocInventory->inv.size();i++){
		buffer<< allocInventory->inv[i]->amount <<'\n';
	}

	cout << buffer.str() << endl;
}

typedef struct thread_record {

	pthread_t threadID;
	int minDurationItemtime;
	int startTime;

}
thread_record;

//use values from 51 to 150.
#define MIN_ORDER_PRIORITY 51
#define PRIORITY_RANGE 100
#define ITEM_TIME 1 //if we have items of variable processing time, this is where that change needs to go
#define TIMESCALE 100000 //makes processes slower to simulate processing time; add zeros to make processes slower


//struct for computing competitive ratio of thread performance
typedef struct temporaryRatio{
		double ratio;
		int index;
	}temporaryRatio;

int calcOrderDuration(order* newOrder){
	int itemCount = 0;
	for(int i=0;i< (sizeof(newOrder->item)/sizeof(item_order)); i++){
		if(newOrder->item[i].item!=""){		//Henry: Made some changes here
			itemCount += newOrder->item[i].quantity * ITEM_TIME;
		}
	}

	return itemCount;
}

//priority comparator for following algorithm
bool priorityComparator(temporaryRatio a, temporaryRatio b){
	return (a.ratio < b.ratio);
}


//priority setting algorithm
void prioAlgorithm(vector<thread_record>* thread_record_collection) {
	int threadCount = sizeof(thread_record_collection)/sizeof(thread_record);

	//change this to vector maybe
	temporaryRatio ratios[threadCount];

	//calculates current competitive ratios, recording index to remember after sorting
	for(int i = 0; i < threadCount; i++){
		ratios[i].ratio = ( clock() - thread_record_collection->at(i).startTime) / thread_record_collection->at(i).minDurationItemtime;
		ratios[i].index = i;
	}

	std::sort(ratios, ratios+threadCount, priorityComparator);

	int newPriority=MIN_ORDER_PRIORITY;
	//assign new priorities
	vector<int> threadsToRemove;
	for(int i = 0; i < threadCount; i++){
		newPriority = (PRIORITY_RANGE / threadCount) * i;
		//updates priority, if check succeeds it means the thread has already ended
		if(pthread_setschedprio(thread_record_collection->at(ratios[i].index).threadID, MIN_ORDER_PRIORITY+newPriority) == ESRCH){
			threadsToRemove.push_back(ratios[i].index);
		}
	}
	std::sort(threadsToRemove.begin(),threadsToRemove.end(),greater<int>());
	for(int i = 0; i < threadsToRemove.size(); i++){
		thread_record_collection->erase(thread_record_collection->begin()+threadsToRemove[i]);
	}

	return;
}

//finds inventory index of particular item
int indexOf(item_order* currentItem){
	//cout << "indexOf Called" << endl;
	for(int i = 0; i < regularInventory->inv.size(); i++){
		if(currentItem->item == regularInventory->inv.at(i)->prodName){
			//cout << "returning " << i << endl;
			return i;
		}
	}
	cout << "Could not find Item!!!!" << endl;
	return -1;
 }

//helper function for processOrder
void processItem(item_order* currentItem){
	int index = indexOf(currentItem);


	pthread_mutex_lock(&regularInventoryMutexes[index]);
	pthread_mutex_lock(&allocInventoryMutexes[index]);

	regularInventory->inv.at(index)->amount -= currentItem->quantity;
	if(regularInventory->inv.at(index)->amount < 0){
		cout << "Negative Inventory Error" << endl;
	}

	allocInventory->inv.at(index)->amount -= currentItem->quantity;


	pthread_mutex_unlock(&regularInventoryMutexes[index]);
	pthread_mutex_unlock(&allocInventoryMutexes[index]);
}


int uniqueItems(order* currentOrder){
	int uniqueItems=0;
	for(int i=0;i< (sizeof(currentOrder->item)/sizeof(item_order)); i++){
			if(currentOrder->item[i].item!=""){
				uniqueItems=uniqueItems+1;
		}
	}
	return uniqueItems;
}


void * processOrder(void * arg) {
	clock_t startTime = clock();

	order currentOrder = *(order*)arg;

	int orderDuration = calcOrderDuration(&currentOrder);

	//locks cpu until timer up
	int timer = 0;
	while( timer < orderDuration*TIMESCALE ){
		timer++;
	}

	//Get # of Unique Items (to calculate size of list)
	int itemsLength=uniqueItems(&currentOrder);

	for(int i = 0; i < itemsLength; i++){
		processItem(currentOrder.item+i);
	}

	clock_t endTime = clock();

	//printf used to eliminate a race condition with cout of other threads
	printf("Order %d completed with ratio: %d.\n", currentOrder.orderNo, (endTime - startTime) / orderDuration);

    return NULL;
}


bool acceptOrder(order* currentOrder){

	int itemsLength = uniqueItems(currentOrder);

	for(int i=0 ; i < itemsLength; ++i){
		item_order temp = currentOrder->item[i];

		int itemIndex = indexOf(&temp);

		pthread_mutex_lock(&regularInventoryMutexes[itemIndex]);
		pthread_mutex_lock(&allocInventoryMutexes[itemIndex]);


		if(regularInventory->inv.at(itemIndex)->amount  -  allocInventory->inv.at(itemIndex)->amount  <  currentOrder->item[i].quantity){
			cout << "Order rejected, not enough inventory" << endl;

			pthread_mutex_unlock(&regularInventoryMutexes[itemIndex]);
			pthread_mutex_unlock(&allocInventoryMutexes[itemIndex]);

			for(int j=0; j < i; j++){
				pthread_mutex_lock(&allocInventoryMutexes[indexOf(&currentOrder->item[j])]);

				allocInventory->inv.at(indexOf(&currentOrder->item[j]))->amount -= currentOrder->item[j].quantity;

				pthread_mutex_unlock(&allocInventoryMutexes[indexOf(&currentOrder->item[j])]);
			}

			return false;
		}

		allocInventory->inv.at(itemIndex)->amount += currentOrder->item[i].quantity;


		pthread_mutex_unlock(&regularInventoryMutexes[itemIndex]);
		pthread_mutex_unlock(&allocInventoryMutexes[itemIndex]);
	}

	return true;
}

int main() {

	pthread_t selfThreadID = pthread_self();
	pthread_setschedprio(selfThreadID, 200);


	vector<thread_record> order_threads;

	mutexInit(&regularInventoryMutexes);
	mutexInit(&allocInventoryMutexes);

    //server loop
    name_attach_t * attach;
    if ((attach = name_attach(NULL, "BCHT_Warehouse_Channel", 0)) == NULL) {
       printf("Failed to attach to client.\n");
       return EXIT_FAILURE;
    }

    while (1) {
    	cout << "Waiting for client message." << endl;
    	order order1;

       int out_status = -1;
       int rcvid = MsgReceive(attach -> chid, & order1, sizeof(order), NULL);


       //Message Receive Error
       if (rcvid == -1) {
          cout << "MsgReceive Error: " << endl;

       //Message Received
       } else if (rcvid > 0) {
    	   	 if(acceptOrder(&order1)){
				 out_status=1;
				 //create thread record
				 thread_record newThreadRecord;
				 newThreadRecord.startTime = clock();
				 //determine minimum runtime of order
				 newThreadRecord.minDurationItemtime = calcOrderDuration(&order1);

				 //create thread
				 pthread_create(&newThreadRecord.threadID, NULL, &processOrder, (void *)&order1);
				 order_threads.push_back(newThreadRecord);

				 //re-prioritize threads to accommodate new order
				 prioAlgorithm(&order_threads);

				 cout << "Order Accepted" << endl;
    	   	 }
    	   	 else{
    	   		 out_status=-1;
    	   	 }



 		  //send reply
          MsgReply(rcvid, 0, & out_status, sizeof(out_status));

       //Pulse Received
       } else if (rcvid == 0) {
          cout << "Client disconnected." << endl;
       }
    }


    return 0;
}
