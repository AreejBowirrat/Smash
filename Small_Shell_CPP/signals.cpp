#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
using namespace std;

void ctrlZHandler(int sig_num)
{

  SmallShell &smash_inst = SmallShell::getInstance();
  cout << "smash: got ctrl-Z" << endl;
  // JobsList::JobEntry* job = smash_inst.FG_jobs->getLastJob();
  if(smash_inst.fg_job_pid != -1)
  {

  
  smash_inst.jobs_list->addJob((smash_inst.fg_job_cmd).c_str(), smash_inst.fg_job_pid, smash_inst.fg_job_id, true);
  if (kill(smash_inst.fg_job_pid, SIGSTOP) == -1)
  {
    perror("smash error: kill failed");
    
    return;
  }
  else
  {
    
    cout << "smash: process " << smash_inst.fg_job_pid << " was stopped" << endl;
    smash_inst.fg_job_pid=-1;
    smash_inst.fg_job_id=-1;
    smash_inst.fg_job_cmd="";
  }
  }
  return;
}

void ctrlCHandler(int sig_num)
{
  SmallShell &smash_inst = SmallShell::getInstance();
  cout << "smash: got ctrl-C" << endl;
//cout<<"smash_inst.fg_job_pid"<<smash_inst.fg_job_pid<<endl;
 // int last_fg_id = job->job_pid;
 if(smash_inst.fg_job_pid==-1)
 {
   return;
 }
  if (kill(smash_inst.fg_job_pid, SIGKILL) == -1)
  {
    perror("smash error: kill failed");
    return;
  }
  else
  {
   // smash_inst.FG_jobs->removeJobById(job->job_id);
   
    cout << "smash: process " << smash_inst.fg_job_pid << " was killed" << endl;
     smash_inst.fg_job_pid=-1;
    smash_inst.fg_job_id=-1;
    smash_inst.fg_job_cmd="";
    return;
  }
  return;
}


void alarmHandler(int sig_num){
  SmallShell &smash_inst = SmallShell::getInstance();
  //cout << "smash: got an alarm" << endl;
  time_t *time_tlok = new time_t;
  time_t curr_time = time(time_tlok);
  std::vector<pid_t> *jobs_to_remove = new std::vector<pid_t>();
  int count = 0;

  for (auto to_iterator = smash_inst.timeout_list->to_commands_vec->begin(); to_iterator != smash_inst.timeout_list->to_commands_vec->end(); to_iterator++)
  {
    double time_diff = difftime(curr_time, (*to_iterator)->to_command_timestamp);
    if(time_diff >= (*to_iterator)->to_command_duration){
      cout << "smash: got an alarm" << endl;
      pid_t timed_job_pid = (*to_iterator)->to_command_pid;
      int aa = 1;
      int *a = &aa;
      pid_t pid_not_exists = waitpid(timed_job_pid, a, WNOHANG);
      //std::cout <<"pid is "<<pid_exists<<std::endl;
      if(pid_not_exists){
        if(!(timed_job_pid == smash_inst.fg_job_pid)){
          smash_inst.jobs_list->removeJobByPid(timed_job_pid);
         }
        else{
          smash_inst.fg_job_pid=-1;
          smash_inst.fg_job_id=-1;
          smash_inst.fg_job_cmd="";
        }
      }
      
      if(smash_inst.jobs_list->getJobByPid(timed_job_pid) || smash_inst.fg_job_pid == timed_job_pid){
        if (kill(timed_job_pid, SIGKILL) == -1){
        perror("smash error: kill failed");
        return;
        }
        cout <<"smash: "<<"timeout "<<(*to_iterator)->to_command_duration <<" " <<(*to_iterator)->to_command_cmd_line << " "<<"timed out!"<<endl;
      }
      jobs_to_remove->push_back(timed_job_pid);
      count++;

     if(!(timed_job_pid == smash_inst.fg_job_pid)){
       smash_inst.jobs_list->removeJobByPid(timed_job_pid);
     }
     else{
      smash_inst.fg_job_pid=-1;
      smash_inst.fg_job_id=-1;
      smash_inst.fg_job_cmd="";
     }
    }
  }
  while(count){
    pid_t curr_pid = *(jobs_to_remove->begin());
    smash_inst.timeout_list->deleteTimedOutCommand(curr_pid);
    jobs_to_remove->erase(jobs_to_remove->begin());
    count--;
  }
  delete jobs_to_remove;
  smash_inst.timeout_list->alarmForShortestDuration();
  return;
}

