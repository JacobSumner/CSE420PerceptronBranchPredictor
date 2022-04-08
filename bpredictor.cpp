//******************************************
// Author: Alexander Amaya
// Email: amamaya2@asu.edu
// Purpose: CSE 420 Lab 2
// Description: Creates a tournament branch
// predictor using Intel Pin tools. 
//******************************************
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <fstream> 
#include "pin.H"
using namespace std;

static UINT64 takenCorrect = 0;
static UINT64 takenIncorrect = 0;
static UINT64 notTakenCorrect = 0;
static UINT64 notTakenIncorrect = 0;



class BranchPredictor {

  public:
  BranchPredictor() { }

  virtual BOOL makePrediction(ADDRINT address) {return FALSE;};

  virtual void makeUpdate(BOOL takenActually, BOOL takenPredicted, ADDRINT address) {};

};

int global_index = 0;

//One bit predictor based on a finite state machine
struct oneBitPredictor{
  bool prediction = true;
  
  void update_prediction(bool taken_actuallly){
    if(prediction != taken_actuallly){
      prediction = taken_actuallly;
    }
  }

  bool get_prediction(UINT64 address){
    return prediction;
  }
};

//two bit predictor based on a finite state machine
struct twoBitPredictor{

  UINT8 state = 0;

  void update_prediction(bool taken_actually){
    bool prediction = true;
      if(state < 2){prediction = true;}
      else{prediction = false;}

    if(taken_actually != prediction){
      
      if(state < 2){state++;}
      else{state--;}

    }else{

      if(state < 2 && state > 0){state--;}
      else if(state > 1 && state < 3){state++;}

    }

  }

  bool get_prediction(UINT64 address){
       if(state < 2){return true;}
       else{return false;}
  }
};

//Three bit predictor based on a 3bit finite state machine
struct threeBitPredictor{

  UINT8 state = 0;

  void update_prediction(bool taken_actually){
    bool prediction = true;
      if(state < 4){prediction = true;}
      else{prediction = false;}

    if(taken_actually != prediction){

      if(state < 4){state++;}
      else{state--;}

    }else{

      if(state < 4 && state > 0){state--;}
      else if(state > 3 && state < 7){state++;}

    }
  }

  bool get_prediction(UINT64 address){
       if(state < 4){return true;}
       else{return false;}
  }
};

struct correlatingPredictor{
  //Stores 12 bits of history information per index; all initially set to taken
  UINT16 last_results[512] = {4096-1};

  threeBitPredictor threeList[4096];

  void update_prediction(bool taken_actually, UINT64 address){
    // Update prediction
    threeList[last_results[address%512]].update_prediction(taken_actually);
    
    // Update history 
    if(taken_actually){
      last_results[address%512] = ((last_results[address%512] << 1) > 4096 -1)? ((last_results[address%512] << 1) - 4096 )+1 : (last_results[address%512] << 1) + 1;
    }else{
      last_results[address%512] = ((last_results[address%512] << 1) > 4096 -1 )? (last_results[address%512] << 1) - 4096 : (last_results[address%512] << 1);
    }

  }

  bool get_prediction(UINT64 address){
  
    return threeList[last_results[address%512]].get_prediction(address);

  }

};

struct gshare{
  //Stores 12 bits of history information; all initially set to taken
  UINT16 last_results = 4096-1;

  threeBitPredictor threeList[4096];


  void update_prediction(bool taken_actually, UINT64 address){

    // Update prediction
    threeList[(last_results) ^ (address%4096)].update_prediction(taken_actually);
    
    // Update last results 
    if(taken_actually){
      last_results = ((last_results << 1) > (4096 -1))? ((last_results << 1) - 4096) + 1 : (last_results << 1) + 1;
    }else{
      last_results = ((last_results << 1) > (4096-1))? (last_results << 1) - 4096: (last_results << 1);
    }
  }

  bool get_prediction(UINT64 address){
    return threeList[(last_results) ^ (address%4096)].get_prediction(address);

  }

};

struct tournament{

  UINT8 selector[512] = {0};

  correlatingPredictor localPredictor;
  gshare globalPredictor;


  void update_prediction(bool taken_actually, UINT64 address){

    //update the selector value
    if( (taken_actually != localPredictor.get_prediction(address)) && (taken_actually == globalPredictor.get_prediction(address))){

      if(selector[address%512] < 3){selector[address%512]++;}


    }else if((taken_actually == localPredictor.get_prediction(address)) && (taken_actually != globalPredictor.get_prediction(address))){

      if(selector[address%512] > 0){selector[address%512]--;}

    }

    //update the both predictors based on the last results
    localPredictor.update_prediction(taken_actually, address);
    globalPredictor.update_prediction(taken_actually, address);

  }

  bool get_prediction(UINT64 address){
    // If selector is less than 2 the the local predictor's prediction is returned; else global prediction is returned. 
    if(selector[address%512] < 2){return localPredictor.get_prediction(address);}
    else{return globalPredictor.get_prediction(address);}

  }


};

class myBranchPredictor: public BranchPredictor {
  public:
  myBranchPredictor() {

  }

  tournament predictor;

  BOOL makePrediction(ADDRINT address){

    return predictor.get_prediction(address);

  }

  void makeUpdate(BOOL takenActually, BOOL takenPredicted, ADDRINT address){

    predictor.update_prediction(takenActually,address);

  }
  
};

myBranchPredictor* BP;


// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.out", "specify the output file name");


// In examining handle branch, refer to quesiton 1 on the homework
void handleBranch(ADDRINT ip, BOOL direction)
{
  
  BOOL prediction = BP->makePrediction(ip);
  BP->makeUpdate(direction, prediction, ip);

  if(prediction && direction){

    takenCorrect++;

  }else if(!prediction && direction){

    notTakenIncorrect++;

  }else if(prediction && !direction){

    takenIncorrect++;

  }else{

    notTakenCorrect++;

  }

}


void instrumentBranch(INS ins, void * v)
{   
  if(INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
    INS_InsertCall(
      ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
      IARG_INST_PTR,
      IARG_BOOL,
      TRUE,
      IARG_END); 

    INS_InsertCall(
      ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
      IARG_INST_PTR,
      IARG_BOOL,
      FALSE,
      IARG_END);
  }
}
 

/* ===================================================================== */
VOID Fini(int, VOID * v)
{ 
  // double sum = (takenCorrect+takenIncorrect+notTakenCorrect+notTakenIncorrect);
  // double sum_correct = takenCorrect+notTakenCorrect;
  // double percent = sum_correct/sum * 100;
  
  //cout << "\nPercent Accuracy: " << percent <<endl;
  //cout << "Size of Predictor: " << sizeof(BP->predictor) << " bytes out of 4125 bytes "<<endl;


  ofstream outfile;
  outfile.open(KnobOutputFile.Value().c_str());
  outfile.setf(ios::showbase);
  outfile << "takenCorrect: "<< takenCorrect <<"  takenIncorrect: "<< takenIncorrect <<" notTakenCorrect: "<< notTakenCorrect <<" notTakenIncorrect: "<< notTakenIncorrect <<"\n";
  outfile.close();
}


// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    // Make a new branch predictor
    BP = new myBranchPredictor();

    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(instrumentBranch, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

//correlating 94.8
//94.6

