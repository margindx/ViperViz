// viper_queue.cpp


#include "viper_queue.h"
#include "ViperInterface.h"
#include <iostream>

using namespace std;

viper_queue::viper_queue(){}


viper_queue::viper_queue(viper_queue const& other){

  lock_guard<mutex> lock(other.mut);
  data_queue=other.data_queue;

}

viper_queue::~viper_queue(){}

void viper_queue::init(uint32_t* pi){
  keep_on=pi;
}

void viper_queue::push(uint8_t* data,uint32_t count){
  uint32_t i;
  lock_guard<mutex> lock(mut);

  for (i=0;i<count;i++)
	 data_queue.push(data[i]);
  data_cond.notify_one();
}

uint32_t viper_queue::wait_and_pop(uint8_t* buf,uint32_t max){

  uint32_t preamble,size,i;
  if (max<2)
    return 0;  // not enough room to do anything
  unique_lock<mutex> lock(mut);
  if (! data_cond.wait_for(lock,chrono::milliseconds(10),[this]{return !data_queue.empty();}))
    return 0;

  for (i=0;i<8;i++){		// grab the first 2 words
    buf[i]=data_queue.front();
    data_queue.pop();
  }
  
  preamble=*(uint32_t*)buf;
  size=*(uint32_t*)(buf+4);


  if ((preamble!=VIPER_PNO_PREAMBLE)&&(preamble!=VIPER_CMD_PREAMBLE)){	// problem
    queue<uint8_t>().swap(data_queue);	// clear the queue
    return 0;
  }

  // otherwise OK get the rest
  // buf large enough?
  if (max<(size+8))
      return 0;

 
  if (size>data_queue.size()){	// problem
      queue<uint8_t>().swap(data_queue);
      return 0;
  }
    
  for (i=0;i<size;i++){
    buf[i+8]=data_queue.front();
    data_queue.pop();
  }

  return size+8;

}

bool viper_queue::empty() const{

  lock_guard<mutex> lock(mut);
  return data_queue.empty();
}
