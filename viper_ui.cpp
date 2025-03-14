// viper_ui.cpp

#include <iostream>
#include <unistd.h>   //_getch
#include <termios.h>  //_getch
#include <string.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>
#include <condition_variable>

#include "ViperInterface.h"
#include "viper_usb.h"
#include "viper_queue.h"
#include "viper_ui.h"

using namespace std;

const uint32_t CRC_SIZE=sizeof(uint32_t);


thread cont_thread;
const uint32_t CMD_DELAY=200;
//mutex mut_lock;




viper_ui::viper_ui(){
  keep_reading_usb=1;
  is_continuous=0;
  cmd_queue.init(&keep_reading_usb);
  pno_queue.init(&is_continuous);
    //pno_queue.init(keep_reading_usb);
  
}


int viper_ui::detect_input(){

  char input;
  uint32_t rv;

  viper_usb viper;
  int keep_looping=1;

  if (viper.usb_connect()!=0){
    cout<<"\n\nError connecting to Viper over USB\nExiting Viper Simple Terminal\n\n";
    return -1;
  }

  thread read_usb_thread=thread(&viper_ui::read_usb_data,this,&viper);	// start up the read thread

  cout<<"\n\n\nViper Simple Terminal\nCopyright (C) Polhemus 2018\nSample code to demonstrate Viper Interface\n\n\n";

  help_menu();

  while (keep_looping){

    cout<<"Viper >> ";
    
    input=getch();

    cout<<endl;
    
    switch (input){
    case 'C':
    case 'c':
      if (!is_continuous){
	rv=start_continuous(&viper);
	if (rv!=0)
	  cout<<"Error starting continuous ("<<rv<<")\n";
     }
      break;

    case 'F':
    case 'f':
        if (!is_continuous){
            rv=start_continuous_publish(&viper, false);
            if (rv!=0)
                cout<<"Error starting continuous publishing ("<<rv<<")\n";
        }
        break;

    case 'S':
    case 's':
        if (!is_continuous){
            rv=start_continuous_publish(&viper, true);
            if (rv!=0)
                cout<<"Error starting continuous publishing ("<<rv<<")\n";
        }
        break;

    case 'H':
    case 'h':
      help_menu();
      break;

    case 'P':
    case 'p':
      rv=print_single(&viper,is_continuous);
      if (rv!=0)
	cout<<"Error with single print or reset continuous ("<<rv<<")\n";
      else
	is_continuous=0;
      break;

    case 'W':
    case 'w':
      rv=whoami(&viper);
      if (rv!=0)
	cout<<"Error with WhoAmI command ("<<rv<<")\n\n";
      break;

    case 0x18:	// ^X
      keep_looping=0;
      break;
      
    default:
      cout<<"\n"<<"Input not supported\n\n";
    }



  }

  keep_reading_usb=0;
  read_usb_thread.join();
  cout<<"\n\nExiting Terminal\n";
  
  return 0;

}



char viper_ui::getch(){
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
        perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        perror ("tcsetattr ~ICANON");
    printf("%c",buf);	// uncomment this line to print input to terminal
    return buf;
 }


void viper_ui::help_menu(){

  cout<<"\n\nEnter one of the following:\n\nS -- Start continuous publishing on port 5555 to sensor/*\nP -- Single frame of data... or stop continuous streaming\nC -- Start continuous streaming\nW -- WhoAmI command\nH -- This Menu\n^X -- Exit\n\n";

}

uint32_t viper_ui::CalcCrc16(uint8_t *b, uint32_t len)
{
  uint32_t crc = 0;
  while (len--)
    crc = crctable[(crc ^ *b++) & 0xff] ^ (crc >> 8);
  return crc;

}

uint32_t viper_ui::whoami(viper_usb* pvpr){

  uint32_t rv=0;
  const  uint32_t HDR_END_LOC=sizeof(SEUCMD_HDR);
  uint32_t crc,br;

  uint32_t cmd_size=sizeof(SEUCMD_HDR)+CRC_SIZE;
  uint8_t* cmd_pkg=new uint8_t[cmd_size];

  uint32_t resp_size=cmd_size+sizeof(WHOAMI_STRUCT);
  uint8_t* resp_pkg=new uint8_t[resp_size];


  
  SEUCMD_HDR* phdr=(SEUCMD_HDR*)cmd_pkg;

  memset(cmd_pkg,0,sizeof(SEUCMD));
  phdr->preamble=VIPER_CMD_PREAMBLE;
  phdr->size=cmd_size-8;  // preamble and size not incl in size
  phdr->seucmd.cmd=CMD_WHOAMI;
  phdr->seucmd.action=CMD_ACTION_GET;

  crc=CalcCrc16(cmd_pkg,cmd_size-4);  // remove crc size from length
  memcpy(cmd_pkg+HDR_END_LOC,&crc,CRC_SIZE);
  
  pvpr->usb_send_cmd(cmd_pkg,cmd_size);
  this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY));
  br=cmd_queue.wait_and_pop(resp_pkg,resp_size);


  delete[] cmd_pkg;
  if (br==0)
    return 3;

  // check crc of response
  crc=CalcCrc16(resp_pkg,br-4);
  if (crc!= *(uint32_t*)(resp_pkg+(br-4))){
    delete[] resp_pkg;
    return 1;  // failed
  }
  // make sure you get an ack
  if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK){
    delete[] resp_pkg;
    return 2;
  }

  // good response print it out
  WHOAMI_STRUCT* pwho=(WHOAMI_STRUCT*)(resp_pkg+HDR_END_LOC);
  cout<<"WhoAmI Response:\n\n";
  cout<<"device name: "<<pwho->device_name<<"\n";
  cout<<"hardware serial no: "<<pwho->hw_ser_no<<"\n";
  cout<<"ioproc_pn: "<<pwho->ioproc_pn<<"\n";
  cout<<"dsp boot firmware: "<<pwho->dsp_bt_fw_pn<<"\n";
  cout<<"dsp appl firmware: "<<pwho->dsp_app_fw_pn<<"\n\n\n";

  delete[] resp_pkg;

  

  return rv;

}


uint32_t viper_ui::print_single(viper_usb* pvpr,uint32_t reset_cont){

  uint32_t rv=0;
  const  uint32_t HDR_END_LOC=sizeof(SEUCMD_HDR);
  uint32_t crc,br,num_sens,i;

  uint32_t cmd_size=sizeof(SEUCMD_HDR)+CRC_SIZE;
  uint8_t* cmd_pkg=new uint8_t[cmd_size];

  uint32_t resp_size=sizeof(uint32_t)*11+sizeof(SENFRAMEDATA)*16+CRC_SIZE; // max no. of sensors
  uint8_t* resp_pkg=new uint8_t[resp_size];
 

  
  SEUCMD_HDR* phdr=(SEUCMD_HDR*)cmd_pkg;
  SENFRAMEDATA* pfd;

  memset(cmd_pkg,0,cmd_size);
  phdr->preamble=VIPER_CMD_PREAMBLE;
  phdr->size=cmd_size-8;  // preamble and size not incl in size
  
  if (reset_cont){  // shutting down continuous
    is_continuous=0;
    phdr->seucmd.cmd=CMD_CONTINUOUS_PNO;
    phdr->seucmd.action=CMD_ACTION_RESET;
    crc=CalcCrc16(cmd_pkg,cmd_size-CRC_SIZE);
    memcpy(cmd_pkg+HDR_END_LOC,&crc,CRC_SIZE);

    do{
      pvpr->usb_send_cmd(cmd_pkg,cmd_size);
      this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY));
      br=cmd_queue.wait_and_pop(resp_pkg,32);
    } while(!br);
 
    delete[] cmd_pkg;
    // check crc of response
    crc=CalcCrc16(resp_pkg,br-4);
    if (crc!= *(uint32_t*)(resp_pkg+(br-CRC_SIZE))){
      delete[] resp_pkg;
      cout<<"crc fail\n";
      return 1;  // failed
    }
    
    delete[] resp_pkg;
    // check for ack
    if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK){
      cout<<"no ack\n";
      return 2;
    }

    //    is_continuous=0;
    cont_thread.join();
    return 0;
  }

  // Single Frame
  phdr->seucmd.cmd=CMD_SINGLE_PNO;
  phdr->seucmd.action=CMD_ACTION_GET;

  crc=CalcCrc16(cmd_pkg,cmd_size-CRC_SIZE);  
  memcpy(cmd_pkg+HDR_END_LOC,&crc,CRC_SIZE);

  pvpr->usb_send_cmd(cmd_pkg,cmd_size);
  this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY));
  br=cmd_queue.wait_and_pop(resp_pkg,resp_size);

  delete[] cmd_pkg;


  // make sure you get an ack
  if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK)
     return 2;
  
  // check CRC
  crc=CalcCrc16(resp_pkg,br-CRC_SIZE);
  if (crc!=*(uint32_t*)(resp_pkg+br-CRC_SIZE)){
    delete[] resp_pkg;
    return 3;
  }
    
  num_sens=*(uint32_t*)(resp_pkg+40);

  pfd=(SENFRAMEDATA*)(resp_pkg+44);
  for (i=0;i<num_sens;i++)
    print_pno_record(pfd+i);
  cout<<"\n";

  delete[] resp_pkg;

  return rv;
}

uint32_t viper_ui::start_continuous(viper_usb* pvpr){

  uint8_t cmd_pkg[32];
  uint8_t resp_pkg[32];
  uint32_t crc;

  memset(cmd_pkg,0,32);
  SEUCMD_HDR* phdr=(SEUCMD_HDR*)cmd_pkg;
  phdr->preamble=VIPER_CMD_PREAMBLE;
  phdr->size=24;  // preamble and size not incl in size
  phdr->seucmd.cmd=CMD_CONTINUOUS_PNO;
  phdr->seucmd.action=CMD_ACTION_SET;

  crc=CalcCrc16(cmd_pkg,28);
  memcpy(cmd_pkg+28,&crc,CRC_SIZE);

  pvpr->usb_send_cmd(cmd_pkg,32);
  this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY));
  cmd_queue.wait_and_pop(resp_pkg,32);


  // check CRC
  crc=CalcCrc16(resp_pkg,28);
  if (crc!=*(uint32_t*)(resp_pkg+28))
    return 1;

  // check for ack
  if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK)
    return 2;

  // start collection thread
  is_continuous=1;
  cont_thread=thread(&viper_ui::stream_cont,this,pvpr);
  
  return 0;

  
}

 void viper_ui::read_usb_data(viper_usb* pvpr){  // thread to constantly poll usb for incoming data

  uint32_t resp_size=sizeof(uint32_t)*11+sizeof(SENFRAMEDATA)*16+CRC_SIZE; // max no. of sensors
  uint8_t* resp_pkg=new uint8_t[resp_size];
  int br;
 
  uint32_t t_o=200;
  viper_queue* pqueue;

  
  while (keep_reading_usb){
    br=pvpr->usb_rec_resp(resp_pkg,resp_size);
    if (br){
      if (*(uint32_t*)resp_pkg==VIPER_PNO_PREAMBLE){
	pqueue=&pno_queue;
	t_o=2;
      }
      else 
	pqueue=&cmd_queue;

      pqueue->push(resp_pkg,br);
     }

    this_thread::sleep_for(std::chrono::milliseconds(t_o));
  }
}



void viper_ui::stream_cont(viper_usb* pvpr){

  const  uint32_t HDR_END_LOC=24;
  uint32_t crc,br,num_sens,i;


  uint32_t resp_size=sizeof(uint32_t)*11+sizeof(SENFRAMEDATA)*16+CRC_SIZE; // max no. of sensors
  uint8_t* resp_pkg=new uint8_t[resp_size];

  cout<<"starting continuous, value= "<<is_continuous<<"\n";

   SENFRAMEDATA* pfd;

  while (is_continuous){


    br=pno_queue.wait_and_pop(resp_pkg,resp_size);

    if (br && (br==(*(uint32_t*)(resp_pkg+4)+8))){  // did we read it all?
	// check crc
	crc=CalcCrc16(resp_pkg,br-4);
	if (crc==*(uint32_t*)(resp_pkg+br-4)){
	  num_sens=*(uint32_t*)(resp_pkg+20);
	  pfd=(SENFRAMEDATA*)(resp_pkg+HDR_END_LOC);
	  cout<<"Frame: "<<*(uint32_t*)(resp_pkg+12)<<"\n";
	  for (i=0;i<num_sens;i++)
	    print_pno_record(pfd+i);
	  cout<<"\n";
	}
      }
    this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  // clean out any residue
  br=pno_queue.wait_and_pop(resp_pkg,resp_size);

  

}

void  viper_ui::print_pno_record(SENFRAMEDATA* pfd){

  uint32_t sens=(pfd->SFinfo.bfSnum&0xff)+1;

//  cout<<sens<<"  "<<pfd->pno.pos[0]<<"  "<<pfd->pno.pos[1]<<"  "<<pfd->pno.pos[2]<<"  "<<pfd->pno.ori[0]<<"  "<<pfd->pno.ori[1]<<"  "<<pfd->pno.ori[2]<<"\n";

    cout<<sens<<"  "<<pfd->pno.pos[0] * 25.4 <<"  "<<pfd->pno.pos[1] * 25.4 <<"  "<<pfd->pno.pos[2] * 25.4 <<"  "<<pfd->pno.ori[0]<<"  "<<pfd->pno.ori[1]<<"  "<<pfd->pno.ori[2]<<"\n";



}

SensorData viper_ui::pno_record_to_SensorData(SENFRAMEDATA *pfd) {
    SensorData data{};

    uint32_t sens = (pfd->SFinfo.bfSnum&0xff)+1;
//    std::cout << "Sen id" << sens << ::std::endl;
    data.sensor_id = (int) sens;
    data.time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    data.x = pfd->pno.pos[0] * 0.254;
    data.y = pfd->pno.pos[1] * 0.254;
    data.z = pfd->pno.pos[2] * 0.254;

    data.nx = pfd->pno.ori[0];
    data.ny = pfd->pno.ori[1];
    data.nz = pfd->pno.ori[2];

    SensorData::convertFromEuler(data);

    return data;
}

void viper_ui::publish_pno_record(SENFRAMEDATA *pfd) {
    SensorData data = pno_record_to_SensorData(pfd);

    ZMQServer::publishSensorData(data);
}

void viper_ui::publish_cont(viper_usb *pvpr, bool slow) {
    const  uint32_t HDR_END_LOC=24;
    uint32_t crc,br,num_sens,i;


    uint32_t resp_size=sizeof(uint32_t)*11+sizeof(SENFRAMEDATA)*16+CRC_SIZE; // max no. of sensors
    uint8_t* resp_pkg=new uint8_t[resp_size];

    cout<<"starting continuous, value= "<<is_continuous<<"\n";

    SENFRAMEDATA* pfd;

    int j = 0;
    const int jmax = 15;

    while (is_continuous) {
        br=pno_queue.wait_and_pop(resp_pkg,resp_size);

        if (br && (br==(*(uint32_t*)(resp_pkg+4)+8))){  // did we read it all?
            // check crc
            crc=CalcCrc16(resp_pkg,br-4);
            if (crc==*(uint32_t*)(resp_pkg+br-4)){
                num_sens=*(uint32_t*)(resp_pkg+20);
                pfd=(SENFRAMEDATA*)(resp_pkg+HDR_END_LOC);
                cout<<"Frame: "<<*(uint32_t*)(resp_pkg+12)<<"\n";
                cout << "N sensors: " << num_sens << "\n";
                j++;
                if (slow && (j == jmax)) {
                    j = 0;
                    for (i = 0; i < num_sens; i++) {
                        publish_pno_record(pfd + i);
                        print_pno_record(pfd + i);
                    }
//                    this_thread::sleep_for(std::chrono::milliseconds(1));
                } else if (!slow) {
                    for (i = 0; i < num_sens; i++) {
                        publish_pno_record(pfd + i);
                        print_pno_record(pfd + i);
                    }
                    this_thread::sleep_for(std::chrono::milliseconds(1));
                }
//                cout<<"\n";
            }
        }

//        this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // clean out any residue
    br=pno_queue.wait_and_pop(resp_pkg,resp_size);
}

uint32_t viper_ui::start_continuous_publish(viper_usb *pvpr, bool slow) {
    ZMQServer::init(5555, ZMQServer::Publisher);

    uint8_t cmd_pkg[32];
    uint8_t resp_pkg[32];
    uint32_t crc;

    memset(cmd_pkg,0,32);
    SEUCMD_HDR* phdr=(SEUCMD_HDR*)cmd_pkg;
    phdr->preamble=VIPER_CMD_PREAMBLE;
    phdr->size=24;  // preamble and size not incl in size
    phdr->seucmd.cmd=CMD_CONTINUOUS_PNO;
    phdr->seucmd.action=CMD_ACTION_SET;

    crc=CalcCrc16(cmd_pkg,28);
    memcpy(cmd_pkg+28,&crc,CRC_SIZE);

    pvpr->usb_send_cmd(cmd_pkg,32);
    this_thread::sleep_for(std::chrono::milliseconds(1));
    cmd_queue.wait_and_pop(resp_pkg,32);


    // check CRC
    crc=CalcCrc16(resp_pkg,28);
    if (crc!=*(uint32_t*)(resp_pkg+28))
        return 1;

    // check for ack
    if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK)
        return 2;

    // start collection thread
    is_continuous=1;
    cont_thread=thread(&viper_ui::publish_cont,this,pvpr,slow);

    return 0;
}

//uint32_t viper_ui::set_hemisphere_auto(viper_usb *pvpr) {
//    uint32_t rv=0;
//    const  uint32_t HDR_END_LOC=sizeof(SEUCMD_HDR);
//    uint32_t crc,br;
//
//    uint32_t hemisphere_config_size = sizeof(HEMISPHERE_CONFIG);
//
//    auto hemisphere_config = HEMISPHERE_CONFIG{};
//    hemisphere_config.bf.track_en = 0;
//    hemisphere_config.bf.auto_en = 0;
//    hemisphere_config.params[0] = 0;
//    hemisphere_config.params[1] = 0;
//    hemisphere_config.params[2] = 0;
//
//
//    uint32_t cmd_size=sizeof(SEUCMD_HDR)+hemisphere_config_size+CRC_SIZE;
//    uint8_t* cmd_pkg=new uint8_t[cmd_size];
//
//    uint32_t resp_size=cmd_size+sizeof(HEMISPHERE_CONFIG);
//    uint8_t* resp_pkg=new uint8_t[resp_size];
//
//
//
//    SEUCMD_HDR* phdr=(SEUCMD_HDR*)cmd_pkg;
//
//    memset(cmd_pkg,0,sizeof(SEUCMD));
//    phdr->preamble=VIPER_CMD_PREAMBLE;
//    phdr->size=cmd_size-8;  // preamble and size not incl in size
//    phdr->seucmd.cmd=CMD_HEMISPHERE;
//    phdr->seucmd.action=CMD_ACTION_SET;
//
//    uint32_t crc_count = sizeof(SEUCMD_HDR);
//    memcpy(cmd_pkg, (const void *)this, sizeof(SEUCMD_HDR));
//    const void * ppay = hemisphere_config;
//
//    memcpy(cmd_pkg + sizeof(SEUCMD_HDR), ppay, hemisphere_config_size);
//    crc_count += hemisphere_config_size;
//
//    uint32_t *pcrc = (uint32_t*)&buf[crc_count];
//    *pcrc = CalcCRC_Bytes(buf, crc_count);
//
//    txbytes = crc_count + sizeof(uint32_t);
//
//    crc=CalcCrc16(cmd_pkg,cmd_size-4);  // remove crc size from length
//    memcpy(cmd_pkg+HDR_END_LOC,&crc,CRC_SIZE);
//
//    pvpr->usb_send_cmd(cmd_pkg,cmd_size);
//    this_thread::sleep_for(std::chrono::milliseconds(CMD_DELAY));
//    br=cmd_queue.wait_and_pop(resp_pkg,resp_size);
//
//
//    delete[] cmd_pkg;
//    if (br==0)
//        return 3;
//
//    // check crc of response
//    crc=CalcCrc16(resp_pkg,br-4);
//    if (crc!= *(uint32_t*)(resp_pkg+(br-4))){
//        delete[] resp_pkg;
//        return 1;  // failed
//    }
//    // make sure you get an ack
//    if (*(uint32_t*)(resp_pkg+16)!=CMD_ACTION_ACK){
//        delete[] resp_pkg;
//        return 2;
//    }
//
//    // good response print it out
//    WHOAMI_STRUCT* pwho=(WHOAMI_STRUCT*)(resp_pkg+HDR_END_LOC);
//    cout<<"WhoAmI Response:\n\n";
//    cout<<"device name: "<<pwho->device_name<<"\n";
//    cout<<"hardware serial no: "<<pwho->hw_ser_no<<"\n";
//    cout<<"ioproc_pn: "<<pwho->ioproc_pn<<"\n";
//    cout<<"dsp boot firmware: "<<pwho->dsp_bt_fw_pn<<"\n";
//    cout<<"dsp appl firmware: "<<pwho->dsp_app_fw_pn<<"\n\n\n";
//
//    delete[] resp_pkg;
//
//
//
//    return rv;
//}


