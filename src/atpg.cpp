/**********************************************************************/
/*           Constructors of the classes defined in atpg.h            */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include "atpg.h"

/* constructor of ATPG */
ATPG::ATPG() {
  /* orginally assigned in tpgmain.c */
  this->backtrack_limit = 200;      /* default value */
  this->backtrack_limit_v1 = 100;   /* default value */
  this->v2_loop_limit = 10;
  this->random_sim_num = 30;
  this->random_sim_num_post = 30;
  this->stc_use_sorted = true;

  this->total_attempt_num = 1;      /* default value */
  this->fsim_only = false;          /* flag to indicate fault simulation only */
  this->tdfsim_only = false;        /* flag to indicate tdfault simulation only */
  this->compression = false;        /* flag to indicate test compression on */
  this->detection_num = 1;          /* number of detection */
  this->rank_fault_method = D_SCORE;

  /* orginally assigned in input.c */
  this->debug = 0;                /* != 0 if debugging;  this is a switch of debug mode */
  this->lineno = 0;               /* current line number */
  this->targc = 0;                /* number of args on current command line */
  this->file_no = 0;              /* number of current file */
  
  /* orginally assigned in init_flist.c */
  this->num_of_gate_fault = 0; // totle number of faults in the whole circuit
  
  /* orginally assigned in test.c */
  this->in_vector_no = 0;         /* number of test vectors generated */

  this->v2_loop_max_trial = INT_MIN;
}

/* constructor of WIRE */
ATPG::WIRE::WIRE() {
  this->value = 0;
  this->flag = 0;
  this->level = 0;
  this->wire_value1 = 0;
  this->wire_value2 = 0;
  this->fault_flag = 0;
  this->wlist_index = 0;
  this->value_v1 = 0;
  this->cc0 = 0;
  this->cc1 = 0;
}

/* constructor of NODE */
ATPG::NODE::NODE() {
  this->type = 0;
  this->flag = 0;
}

/* constructor of FAULT */
ATPG::FAULT::FAULT() {
  this->node = nullptr;
  this->io = 0;
  this->index = 0;
  this->fault_type = 0;
  this->detect = 0;
  this->test_tried = false;
  this->eqv_fault_num = 0;
  this->to_swlist = 0;
  this->fault_no = 0;
  this->detected_time = 0;
  this->scoap = 0;
  this->score = 0;
}
