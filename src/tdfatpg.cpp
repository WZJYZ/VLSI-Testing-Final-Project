/**********************************************************************/
/*           Transition Delay Fault ATPG;                             */
/*           building the pattern list;                               */
/*           calculating the total fault coverage.                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/05/2018 created                                */
/**********************************************************************/

#include "atpg.h"

#define RANDOM_PATTERN_NUM 1000000

void ATPG::transition_delay_fault_atpg(void) {
  srand(0); // what's your lucky number?

  backtrack_limit_v1v2 = 1;
  backtrack_limit_v1 = 200;

  string vec;
  int current_detect_num = 0;
  int total_detect_num = 0;
  int total_no_of_backtracks = 0; // accumulative number of backtracks
  int current_backtracks = 0;
  int no_of_aborted_faults = 0;
  int no_of_redundant_faults = 0;
  int no_of_calls = 0;

  calculate_cc();
  calculate_co();

  fptr fault_under_test = select_primary_fault();

  /* generate test pattern */
  while (fault_under_test != nullptr) {
    switch (tdf_podem_v2(fault_under_test, current_backtracks)) {
      case TRUE:
        /* form a vector */
        vec.clear();
        assert(!vectors.empty());
        vec = vectors.back();
        /*by defect, we want only one pattern per fault */
        /*run a fault simulation, drop ALL detected faults */
        if (total_attempt_num == 1) {
          tdfsim_a_vector(vec, current_detect_num);
          total_detect_num += current_detect_num;
        }
        /* If we want mutiple petterns per fault, 
         * NO fault simulation.  drop ONLY the fault under test */ 
        else {
          // fault_under_test->detect = TRUE;
          /* drop fault_under_test */
          flist_undetect.remove(fault_under_test);
        }
        in_vector_no++;
        break;

      case FALSE:
          fault_under_test->detect = REDUNDANT;
          no_of_redundant_faults++;
          break;
    
      case MAYBE:
          no_of_aborted_faults++;
          break;
    }
    fault_under_test->test_tried = true;
    fault_under_test = nullptr;
    for (fptr fptr_ele: flist_undetect) {
      if (!fptr_ele->test_tried) {
        fault_under_test = fptr_ele;
        break;
      }
    }
    total_no_of_backtracks += current_backtracks; // accumulate number of backtracks
    no_of_calls++;
  }

  // display_undetect();

  static_compression();

  display_test_patterns();

  fprintf(stdout,"\n");
  fprintf(stdout,"#number of aborted faults = %d\n",no_of_aborted_faults);
  fprintf(stdout,"\n");
  fprintf(stdout,"#number of redundant faults = %d\n",no_of_redundant_faults);
  fprintf(stdout,"\n");
  fprintf(stdout,"#number of calling podem1 = %d\n",no_of_calls);
  fprintf(stdout,"\n");
  fprintf(stdout,"#total number of backtracks = %d\n",total_no_of_backtracks);
}

/* select a primary fault for podem */
ATPG::fptr ATPG::select_primary_fault()
{
  fptr fault_selected = flist_undetect.front();

  return fault_selected;
}

/* select a secondary fault for podem_x */
ATPG::fptr ATPG::select_secondary_fault()
{
  fptr fault_selected = flist_undetect.front();

  return fault_selected;
}
  
/* generate test vector 2, considering fault/LOS constraints */
int ATPG::tdf_podem_v2(const fptr fault, int& current_backtracks) 
{
  int i,ncktwire,ncktin;
  wptr wpi; // points to the PI currently being assigned
  forward_list<wptr> decision_tree; // design_tree (a LIFO stack)
  wptr wfault;
  int attempt_num = 0;  // counts the number of pattern generated so far for the given fault
  int backtrack_v1v2 = 0;

  /* initialize all circuit wires to unknown */
  ncktwire = sort_wlist.size();
  ncktin = cktin.size();
  for (i = 0; i < ncktwire; i++) {
    sort_wlist[i]->value = U;
    sort_wlist[i]->value_v1 = U;
  }
  no_of_backtracks = 0;
  find_test = false;
  no_test = false;
  
  mark_propagate_tree(fault->node);

  /* Fig 7 starts here */
  /* set the initial goal, assign the first PI.  Fig 7.P1 */
  switch (set_uniquely_implied_value(fault)) {
    case TRUE: // if a  PI is assigned 
      sim();  // Fig 7.3
      wfault = fault_evaluate(fault);
      if (wfault != nullptr) forward_imply(wfault);// propagate fault effect
      if (check_test()) find_test = true; // if fault effect reaches PO, done. Fig 7.10
      break;
    case CONFLICT:
      no_test = true; // cannot achieve initial objective, no test
      break;
    case FALSE:
      break;  //if no PI is reached, keep on backtracing. Fig 7.A 
  }

  /* loop in Fig 7.ABC 
   * quit the loop when either one of the three conditions is met: 
   * 1. number of backtracks is equal to or larger than limit
   * 2. no_test
   * 3. already find a test pattern AND no_of_patterns meets required total_attempt_num */
  while ((no_of_backtracks < backtrack_limit) && !no_test &&
    !(find_test && (attempt_num == total_attempt_num))) {
    
    /* check if test possible.   Fig. 7.1 */
    if (wpi = test_possible(fault)) {
      wpi->flag |= CHANGED;
      /* insert a new PI into decision_tree */
      decision_tree.push_front(wpi);
    }
    else { // no test possible using this assignment, backtrack. 

      while (!decision_tree.empty() && (wpi == nullptr)) {
        /* if both 01 already tried, backtrack. Fig.7.7 */
        if (decision_tree.front()->flag & ALL_ASSIGNED) {
          decision_tree.front()->flag &= ~ALL_ASSIGNED;  // clear the ALL_ASSIGNED flag
          decision_tree.front()->value = U; // do not assign 0 or 1
          decision_tree.front()->flag |= CHANGED; // this PI has been changed
          /* remove this PI in decision tree.  see dashed nodes in Fig 6 */
          decision_tree.pop_front();
        }  
        /* else, flip last decision, flag ALL_ASSIGNED. Fig. 7.8 */
        else {
          decision_tree.front()->value = decision_tree.front()->value ^ 1; // flip last decision
          decision_tree.front()->flag |= CHANGED; // this PI has been changed
          decision_tree.front()->flag |= ALL_ASSIGNED;
          no_of_backtracks++;
          wpi = decision_tree.front();
        }
      } // while decision tree && ! wpi
      if (wpi == nullptr) no_test = true; //decision tree empty,  Fig 7.9
    } // no test possible

/* this again loop is to generate multiple patterns for a single fault 
 * this part is NOT in the original PODEM paper  */
again:  if (wpi) {
      sim();
      if (wfault = fault_evaluate(fault)) forward_imply(wfault);
      if (check_test()) {
        find_test = true;
        /* if multiple patterns per fault, print out every test cube */
        if (total_attempt_num > 1) {
          if (attempt_num == 0) {
            display_fault(fault);
          }
          display_io();
        }
        attempt_num++; // increase pattern count for this fault

		    /* keep trying more PI assignments if we want multiple patterns per fault
		     * this is not in the original PODEM paper*/
        if (total_attempt_num > attempt_num) {
          wpi = nullptr;
          while (!decision_tree.empty() && (wpi == nullptr)) {
            /* backtrack */
            if (decision_tree.front()->flag & ALL_ASSIGNED) {
              decision_tree.front()->flag &= ~ALL_ASSIGNED;
              decision_tree.front()->value = U;
              decision_tree.front()->flag |= CHANGED;
              decision_tree.pop_front();
            }
            /* flip last decision */
            else {
              decision_tree.front()->value = decision_tree.front()->value ^ 1;
              decision_tree.front()->flag |= CHANGED;
              decision_tree.front()->flag |= ALL_ASSIGNED;
              no_of_backtracks++;
              wpi = decision_tree.front();
            }
          }
          if (!wpi) no_test = true;
          assert(0);
          goto again;  // if we want multiple patterns per fault
        } // if total_attempt_num > attempt_num
      }  // if check_test()
    } // again
  } // while (three conditions)

  /* clear everthing */
  for (wptr wptr_ele: decision_tree) {
    wptr_ele->flag &= ~ALL_ASSIGNED;
  }
  decision_tree.clear();
  
  unmark_propagate_tree(fault->node);
  
  if (find_test) {
    /* normally, we want one pattern per fault */
    if (total_attempt_num == 1) {
	  
      for (i = 0; i < ncktin; i++) {
        switch (cktin[i]->value) {
          case 0:
          case 1: break;
          case D: cktin[i]->value = 1; break;
          case B: cktin[i]->value = 0; break;
          // case U: cktin[i]->value = rand()&01; break; // random fill U
        }
      }
      // display_io();

      if (tdf_podem_v1(fault) == TRUE) {
        string vec(ncktin + 1, '0');
        for (i = 0; i < ncktin; i++) {
          vec[i] = itoc(cktin[i]->value_v1);
        }
        vec.back() = itoc(cktin[0]->value);
        vectors.emplace_back(vec);
        return(TRUE);
      }
      else if (backtrack_v1v2++ < backtrack_limit_v1v2) {
        no_of_backtracks = 0;
        find_test = false;
        no_test = false;
        goto again;
      }
    }
    else fprintf(stdout, "\n");  // do not random fill when multiple patterns per fault
  }
  else if (no_test) {
    // fprintf(stdout,"redundant fault...\n");
    return(FALSE);
  }
  else {
    /*fprintf(stdout,"test aborted due to backtrack limit...\n");*/
    return(MAYBE);
  }
  return FALSE;
}     

/* generate test vector 1, injection/activation/propagation */
int ATPG::tdf_podem_v1(const fptr fault) 
{
   int i, ncktin;
  wptr faulty_wire;

  ncktin = cktin.size();
  faulty_wire = sort_wlist[fault->to_swlist];

  /* shift v2, assign to value_v1, set fixed */
  for (i = 1; i < ncktin; ++i) {
    if (cktin[i]->value != U) {
      cktin[i - 1]->value_v1 = cktin[i]->value;
      cktin[i - 1]->fixed = true;
    }
    else {
      cktin[i - 1]->value_v1 = U;
      cktin[i - 1]->fixed = false;
    }
  }

  for (i = 0; i < ncktin; ++i) {
    cktin[i]->flag &= ~MARKED2;
    cktin[i]->flag &= ~CHANGED2;
    cktin[i]->flag &= ~ALL_ASSIGNED2;
  }

  vector<wptr> fanin_cone_wlist; // follow topological order
  vector<wptr> pi_wlist; // only non-fixed PIs

  /* mark all wires in the fanin cone */
  mark_propagate_tree(faulty_wire, fanin_cone_wlist, pi_wlist);

  /* unmark all wires in the fanin cone */
  for (wptr w : fanin_cone_wlist) {
    w->flag &= ~MARKED2;
  }

  auto partial_sim = [&] (vector<wptr> &wlist) -> void {
    for (int i = 0, nwire = wlist.size(); i < nwire; ++i) {
      if (wlist[i]->flag & INPUT) continue;
      evaluate_v1(wlist[i]->inode.front());
    }
    return;
  };

  /* sim fixed value_v1 */
  partial_sim(fanin_cone_wlist);
  if (faulty_wire->value_v1 != U) {
    if (faulty_wire->value_v1 == fault->fault_type) {
      // fprintf(stderr, "TRUE\n");
      return TRUE;
    }
    else {
      // fprintf(stderr, "FALSE\n");
      return FALSE;
    }
  }

  // display_sort_wlist();

  // no decision can be made; return FALSE
  if (pi_wlist.empty()) return FALSE;

  int ret = FALSE;
  int no_of_backtracks = 0;
  wptr current_wire; // must be pi
  int decision_level = 0; // 0 <= decision_level <= pi_wlist.size() - 1

  while (decision_level >= 0 && no_of_backtracks <= backtrack_limit_v1) {
    // reach the last decision level
    if (decision_level >= pi_wlist.size()) {
      --decision_level;
      continue;
    }

    current_wire = pi_wlist[decision_level];

    if (current_wire->value_v1 == U) {
      current_wire->value_v1 = 0;
      current_wire->flag &= ~ALL_ASSIGNED2;
    }
    else if (current_wire->flag & ALL_ASSIGNED2) {
      current_wire->flag &= ~ALL_ASSIGNED2;
      current_wire->value_v1 = U;
      --decision_level;
      continue;
    }
    else {
      current_wire->value_v1 = current_wire->value_v1 ^ 1;
      current_wire->flag |= ALL_ASSIGNED2;
      ++no_of_backtracks;
    }

    partial_sim(fanin_cone_wlist); // forward imply?
    if (faulty_wire->value_v1 == fault->fault_type) {
      ret = TRUE;
      break;
    }
    else if (faulty_wire->value_v1 == U) {
      ++decision_level;
    }
  }

/*
  cerr << pi_wlist.size() << " / " << cktin.size() 
       << " (" << ((double)pi_wlist.size()/(double)cktin.size() * 100) << "%) " << endl;

  fprintf(stderr, "%s\n", (ret == TRUE ? "TRUE" : "FALSE"));
  fprintf(stderr, "faulty wire: %s %d\n", faulty_wire->name.c_str(), fault->fault_type);
  fprintf(stderr, "Fanins:\n");
  for (const wptr w : fanin_cone_wlist) {
    fprintf(stderr, " %s", w->name.c_str());
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "PIs:\n");
  for (const wptr w : pi_wlist) {
    assert(w->flag & INPUT);
    fprintf(stderr, " %s", w->name.c_str());
  }
  fprintf(stderr, "\n");
  getchar();
*/

  return ret;
}

void ATPG::mark_propagate_tree(const wptr w, vector<wptr> &fanin_cone_wlist, vector<wptr> &pi_wlist) {
  if (w->flag & MARKED2) return;

  int i, j, ninode, niwire;
  w->flag |= MARKED2;
  for (i = 0, ninode = w->inode.size(); i < ninode; ++i) {
    for (j = 0, niwire = w->inode[i]->iwire.size(); j < niwire; ++j) {
      mark_propagate_tree(w->inode[i]->iwire[j], fanin_cone_wlist, pi_wlist);
    }
  }
  fanin_cone_wlist.emplace_back(w);
  if ((w->flag & INPUT) && (w->value_v1 == U)) pi_wlist.emplace_back(w);
}

/* dynamic test compression by podem-x */
int ATPG::tdf_podem_x()  
{
  return FALSE;
}

/* static test compression */
void ATPG::static_compression()  
{
  int detect_num = 0;

  vector<int> order(vectors.size());
  vector<bool> dropped(vectors.size(), false);
  iota(order.begin(), order.end(), 0);
  reverse(order.begin(), order.end());

  for (int i = 0; i < 20; ++i) {
    generate_fault_list();
    int drop_count = 0;
    for (int s : order) {
      if (!dropped[s]) {
        tdfsim_a_vector(vectors[s], detect_num);
        if (detect_num == 0) {
          dropped[s] = true;
          ++drop_count;
        }
      }
    }
    // cerr << drop_count << endl;
    random_shuffle(order.begin(), order.end());
  }

  vector<string> compressed_vectors;
  for (size_t i = 0; i < vectors.size(); ++i) {
    if (!dropped[i]) {
      compressed_vectors.emplace_back(vectors[i]);
    }
  }
  vectors = compressed_vectors;
}

void ATPG::random_pattern_generation() {
  unordered_set<string> sVector;
  string vec1(cktin.size() + 1, '0');
  string vec2(cktin.size() + 1, '0');
  for (int i = 0; i < RANDOM_PATTERN_NUM; ++i) {
    for (int j = 0; j < cktin.size() + 1; ++j) {
      if (rand() % 2) {
        vec1[j] = '0';
        vec2[j] = '1';
      }
      else {
        vec1[j] = '1';
        vec2[j] = '0';
      }
    }
    if (sVector.find(vec1) == sVector.end()) {
      vectors.emplace_back(vec1);
      vectors.emplace_back(vec2);
      sVector.emplace(vec1);
      sVector.emplace(vec2);
    }
  }
}

void ATPG::forward_imply_v1(const wptr w) {
  int i,nout;

  for (i = 0, nout = w->onode.size(); i < nout; i++) {
    if (w->onode[i]->type != OUTPUT) {
      evaluate_v1(w->onode[i]);
      if (w->onode[i]->owire.front()->flag & CHANGED)
	      forward_imply(w->onode[i]->owire.front()); // go one level further
      w->onode[i]->owire.front()->flag &= ~CHANGED;
    }
  }
}/* end of forward_imply */

ATPG::wptr ATPG::find_pi_assignment_v1(const wptr object_wire, const int& object_level) {
  wptr new_object_wire;
  int new_object_level;
  
  /* if PI, assign the same value as objective Fig 9.1, 9.2 */
  if (object_wire->flag & INPUT) {
    object_wire->value_v1 = object_level;
    return(object_wire);
  }

  /* if not PI, backtrace to PI  Fig 9.3, 9.4, 9.5*/
  else {
    switch(object_wire->inode.front()->type) {
      case   OR:
      case NAND:
        if (object_level) new_object_wire = find_easiest_control_v1(object_wire->inode.front());  // decision gate
        else new_object_wire = find_hardest_control_v1(object_wire->inode.front()); // imply gate
        break;
      case  NOR:
      case  AND:
		// TODO  similar to OR and NAND but different polarity
        if (object_level) new_object_wire = find_hardest_control_v1(object_wire->inode.front());
        else new_object_wire = find_easiest_control_v1(object_wire->inode.front());
        break;
		//  TODO END
      case  NOT:
      case  BUF:
        new_object_wire = object_wire->inode.front()->iwire.front();
        break;
    }

    switch (object_wire->inode.front()->type) {
      case  BUF:
      case  AND:
      case   OR: new_object_level = object_level; break;
      /* flip objective value  Fig 9.6 */
      case  NOT:
      case  NOR:
      case NAND: new_object_level = object_level ^ 1; break;
    }
    if (new_object_wire) return(find_pi_assignment_v1(new_object_wire,new_object_level));
    else return(nullptr);
  }
}/* end of find_pi_assignment */


/* Fig 9.4 */
ATPG::wptr ATPG::find_hardest_control_v1(const nptr n) {
  int i;
  
  /* because gate inputs are arranged in a increasing level order,
   * larger input index means harder to control */
  for (i = n->iwire.size() - 1; i >= 0; i--) {
    if (n->iwire[i]->value_v1 == U) return(n->iwire[i]);
  }
  return(nullptr);
}/* end of find_hardest_control */


/* Fig 9.5 */
ATPG::wptr ATPG::find_easiest_control_v1(const nptr n) {
  int i, nin;
  // TODO  similar to hardiest_control but increasing level order
  for (i = 0, nin = n->iwire.size(); i < nin; i++) {
    if (n->iwire[i]->value_v1 == U) return(n->iwire[i]);
  }
  //  TODO END
  return(nullptr);
}/* end of find_easiest_control */