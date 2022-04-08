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

  virtual BOOL makePrediction(ADDRINT address) { return FALSE;};

  virtual void makeUpdate(BOOL takenActually, BOOL takenPredicted, ADDRINT address) {};

};

class myBranchPredictor: public BranchPredictor 
{
  public:
    // Four-Bit Predictor Array
    UINT8 branchTargetBuffer[1024];// allocating 1024 bits of meomory for the BTB

  myBranchPredictor() 
  {

//--------------Array of Four-Bit Predictors------------------------------------------------------
   // Initializing all predictions in the BTB to Very Strongly Taken
   for (int i = 0; i < 1024; i++)
   {
     branchTargetBuffer[i] = 0;
   }
//------------------------------------------------------------------------------------------------

  }

  // The function will return true if the branch is predicted to be taken and false otherwise.
  // Takes in the address of the branch instruction
  BOOL makePrediction(ADDRINT address)
  { 

//--------------Array of Four-Bit Predictors------------------------------------------------------
   // Index of the array based on the address of the function
   UINT64 arrayIndex;

   arrayIndex = (address & 0x3FF); // Getting bottom 10 bits of the address

  switch(branchTargetBuffer[arrayIndex]) {
    
    // Very Very Strongly Taken
    case 0:
      return TRUE;
      break;
    
    // Very Strongly Taken
    case 1:
      return TRUE;
      break;

    //Strongly Taken
    case 2:
      return TRUE;
      break;

    // Weakly Taken
    case 3:
      return TRUE;
      break;

    // Weakly Not Taken
    case 4:
      return FALSE;
      break;

    // Strongly Not Taken
    case 5:
      return FALSE;
      break;
    
    // Very Strongly Not Taken
    case 6:
      return FALSE;
      break;
    
    // Very Very Strongly Not Taken
    case 7:
      return FALSE;
      break;
    
    default:
      // edge case (Do nothing)
      return TRUE;
  }
//------------------------------------------------------------------------------------------------

  }


  // You will use this information to update your internal branch predictor state as you see fit.
  // address: Branch Address
  // takenActually: Actual direction of the branch
  // takenPredicted: The branches original prediction
  void makeUpdate(BOOL takenActually, BOOL takenPredicted, ADDRINT address)
  {
//--------------Array of Four-Bit Predictors------------------------------------------------------
    UINT64 index;
    index = (address & 0x3FF); // Getting bottom 10 bits of the address

    switch(branchTargetBuffer[index]) 
       {
        case 0:
          if(takenActually != takenPredicted) // Misprediction
                {
                  branchTargetBuffer[index] += 1; // Goes to Very Strongly Taken
                }
                // otherwise do nothing      
          break;
        
        case 1:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] += 1; // Goes to Strongly Taken
          }
          else // Correct Prediction, Update Case to Very Very Strongly Taken
          {
            branchTargetBuffer[index] -=1;
          }
          break;

        case 2:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] += 1; // Goes to weakly taken
          }
          else // Correct Prediction, Update Case to Very Strongly Taken
          {
            branchTargetBuffer[index] -= 1;
          }
          break;

        case 3:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] += 1; // Goes to weakly not taken
          }
          else // Correct Prediction, Update Case to Strongly Taken
          {
            branchTargetBuffer[index] -= 1;
          }
          break;

        case 4:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] -= 1; // Goes to weakly taken
          }
          else // Correct Prediction, Update Case to Strongly Taken
          {
            branchTargetBuffer[index] += 1;
          }
          break;

        case 5:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] -= 1; // Goes to Weakly not taken
          }
          else // Correct Prediction, Update Case to Very Strongly Taken
          {
            branchTargetBuffer[index] += 1;
          }
          break;

        case 6:
          if(takenActually != takenPredicted) // Misprediction
          {
            branchTargetBuffer[index] -= 1; // Goes to Strongly not taken
          }
          else // Correct Prediction, Update Case to Very Very Strongly Taken
          {
            branchTargetBuffer[index] += 1;
          }
          break;
        
        case 7:
          if(takenActually != takenPredicted) // Misprediction
                {
                  branchTargetBuffer[index] -= 1; // Goes to Very Strongly Not Taken
                }
                // otherwise do nothing  
          break;
        
        default:
          // edge case (Do nothing)
          break;
       }
//------------------------------------------------------------------------------------------------

  }
  
};

BranchPredictor* BP;


// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.out", "specify the output file name");


// In examining handle branch, refer to quesiton 1 on the homework
void handleBranch(ADDRINT ip, BOOL direction)
{
  BOOL prediction = BP->makePrediction(ip);
  BP->makeUpdate(direction, prediction, ip);

  // Attempt 1 to improve Simulation Speed (Best choice, sequential)
  prediction ? direction ? takenCorrect++ : takenIncorrect++ : direction ? notTakenIncorrect++ : notTakenCorrect++;

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
  //percent = ;
  ofstream outfile;
  outfile.open(KnobOutputFile.Value().c_str());
  outfile.setf(ios::showbase);
  //outfile << "takenCorrect: "<< takenCorrect <<"  takenIncorrect: "<< takenIncorrect <<" notTakenCorrect: "<< notTakenCorrect <<" notTakenIncorrect: "<< notTakenIncorrect <<" PercentCorrect: " << ((((double)takenCorrect + (double)notTakenCorrect)/((double)takenCorrect + (double)notTakenCorrect + (double)takenIncorrect + (double)notTakenIncorrect))*100) <<"%\n";
outfile << "takenCorrect: "<< takenCorrect <<"  takenIncorrect: "<< takenIncorrect <<" notTakenCorrect: "<< notTakenCorrect <<" notTakenIncorrect: "<< notTakenIncorrect;
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

