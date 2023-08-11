#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#define JOB_ID_INDEX 2
#define SIGNAL_NUM_INDEX 1
#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_SIZE 2
#define BIN_LEN 10
#define C_FLAG_LEN 3
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::to_string;

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

// function declaratio
// static bool checkEof(int fd);
static bool checkNextEofLine(int fd_open_res);
static bool is_number(const string &s);
static int countLines(int fd, int *flag);
static void findPipeSecondAndFirstCommWithNoBg(string cmd_line, unsigned int found_pos, string *first_command, string *second_command);
static void findPipeSecondAndFirstCommWithBG(string cmd_line, unsigned int found_pos, string *first_command, string *second_command);
static void findPipeSecondAndFirstCommWithDoubleRedirection(string cmd_line, unsigned int found_pos, string *first_command, string *second_command);

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == (string::npos)) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == (string::npos)) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == (string::npos))
  {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&')
  {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

SmallShell::SmallShell()
{
  // TODO: add your implementation
  smash_pid = getpid();
  jobs_list = new JobsList();
  // FG_jobs = new JobsList();
  curr_prompt = "smash> ";
  last_updated_directory = "";
  fg_job_id = -1;
  fg_job_pid = -1;
  fg_job_cmd = "";
  num_args = 0;
  args = new char *[COMMAND_MAX_ARGS];
  timeout_list = new TimeOutListClass();
}

Command *SmallShell::CreateCommand(const char *cmd_line)
{

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  this->num_args = _parseCommandLine(cmd_line, this->args);
  bool is_background = _isBackgroundComamnd(cmd_line);
  if (is_background)
  {
    int cmd_line_size = ((string(cmd_line)).size());
    char *str_with_no_sign = new char[cmd_line_size];
    strcpy(str_with_no_sign, cmd_line);
    _removeBackgroundSign(str_with_no_sign);
    cmd_s = _trim(string(str_with_no_sign));
    firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  }
  if (string(cmd_line).find("|") != (string::npos))
  {
    return new PipeCommand(cmd_line);
  }
  else if (string(cmd_line).find(">") != (string::npos))
  {
    return new RedirectionCommand(cmd_line);
  }
  else if (firstWord.compare("pwd&") == 0 || firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid&") == 0 || firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt&") == 0 || firstWord.compare("chprompt") == 0)
  {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("cd&") == 0 || firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs&") == 0 || firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("kill&") == 0 || firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("fg&") == 0 || firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("bg&") == 0 || firstWord.compare("bg") == 0)
  {
    return new BackgroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit&") == 0 || firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("touch") == 0)
  {
    return new TouchCommand(cmd_line);
  }
  else if (firstWord.compare("tail") == 0)
  {
    return new TailCommand(cmd_line);
  }
  else if (firstWord.compare("timeout") == 0)
  {
    return new TimeoutCommand(cmd_line);
  }
  else
  {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}
void SmallShell::executeCommand(const char *cmd_line)
{
  this->jobs_list->removeFinishedJobs();
  Command *cmd = CreateCommand(cmd_line);
  if (cmd == nullptr)
  {
    return;
  }
  cmd->execute();
  return;
}
///////////////////////////////////build-in-commands////////////////////////////
void ChpromptCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  if (smash_inst.num_args == 1)
  {
    smash_inst.curr_prompt = "smash> ";
  }
  else
  {
    smash_inst.curr_prompt = string(smash_inst.args[1]) + "> ";
  }
  return;
}
void ShowPidCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  // if (smash_inst.num_args == 1)
  // {
  cout << "smash pid is " << smash_inst.smash_pid << endl;
  //}
  return;
}
void GetCurrDirCommand::execute()
{
  // SmallShell &smash_inst = SmallShell::getInstance();

  char *buff = new char[COMMAND_ARGS_MAX_LENGTH];
  buff = getcwd(buff, COMMAND_ARGS_MAX_LENGTH);
  if (!buff)
  {
    perror("smash error: getcwd failed");
    return;
  }
  cout << string(buff) << endl;
  return;
}
void ChangeDirCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  string dist_dir;

  if (smash_inst.num_args == 2)
  {
    char *cd_arg_directory = smash_inst.args[1];

    // save the current working directory in buffer
    char *curr_dict_buff = new char[COMMAND_ARGS_MAX_LENGTH];
    curr_dict_buff = getcwd(curr_dict_buff, COMMAND_ARGS_MAX_LENGTH);
    if (curr_dict_buff == nullptr)
    {
      perror("smash error: getcwd failed");
      return;
    }
    //////for cd-
    if (!strcmp("-", cd_arg_directory))
    {
      if (!strcmp("", smash_inst.last_updated_directory.c_str()))
      {
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return;
      }
      dist_dir = smash_inst.last_updated_directory;
    }
    ///// for cd dict or cd ..
    else
    {
      dist_dir = cd_arg_directory;
    }
    /////change the current working directory
    int res = chdir(dist_dir.c_str());
    if (res != 0)
    {
      perror("smash error: chdir failed");
      return;
    }
    // update the last updated working directory in smash
    smash_inst.last_updated_directory = string(curr_dict_buff);
  }
  else if (smash_inst.num_args > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
  }
  return;
}
void JobsCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  smash_inst.jobs_list->printJobsList();
  return;
}
void KillCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();

  if ((smash_inst.num_args != 3) || (atoi(smash_inst.args[SIGNAL_NUM_INDEX]) >= 0) || (atoi(smash_inst.args[JOB_ID_INDEX]) == 0))
  {
    // cerr<<"first if"<<atoi(smash_inst.args[JOB_ID_INDEX])<<endl;
    cerr << "smash error: kill: invalid arguments" << endl;
    return;
  }

  else if (atoi(smash_inst.args[JOB_ID_INDEX]) < 0)
  {
    cerr << "smash error: kill: job-id " << atoi(smash_inst.args[JOB_ID_INDEX]) << " does not exist" << endl;
    return;
  }
  else
  {
    int job_id = atoi(smash_inst.args[JOB_ID_INDEX]);
    JobsList::JobEntry *job = smash_inst.jobs_list->getJobById(job_id);
    if (job == nullptr)
    {
      // cout << "job is nullpteer" << endl;
      cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
      return;
    }
    else
    {
      int signal = (-1) * atoi(smash_inst.args[SIGNAL_NUM_INDEX]);
      if (signal > 31 || signal < 1)
      {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
      }
      int kill_res = kill(job->job_pid, signal);
      if (kill_res != 0)
      {
        //  cout << "kill_res fault"
        //       << " pid is: " << job->job_pid << " the signal is:  " << signal << endl;
        perror("smash error: kill failed");
        return;
      }
      else
      {
        cout << "signal number " << signal << " was sent to pid " << job->job_pid << endl;
        if (signal == SIGKILL)
        {
          job->isFinished = true;
          smash_inst.jobs_list->removeJobById(job->job_id);
        }
        if (signal == SIGCONT)
        {
          job->isStopped = false;
        }
        if (signal == SIGTSTP)
        {
          job->isStopped = true;
        }
        return;
      }
      return;
    }
  }
  return;
}
void ForegroundCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  JobsList::JobEntry *job_to_fg = nullptr;
  if (smash_inst.num_args == 1) // fg the maximal job id
  {
    if (smash_inst.jobs_list->jobs->size() == 0)
    {
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      return;
    }
    else
    {
      // cout << "mazimum job id is: " << (smash_inst.jobs_list->findNewMaxJobAndUpdate())->job_id << endl;
      job_to_fg = smash_inst.jobs_list->findNewMaxJobAndUpdate();
    }
  }
  else if (smash_inst.num_args == 2)
  {
    int job_to_fg_id = atoi(smash_inst.args[1]);
    if (job_to_fg_id == 0)
    {
      cerr << "smash error: fg: invalid arguments" << std::endl;
      return;
    }
    job_to_fg = smash_inst.jobs_list->getJobById(job_to_fg_id);
    if (job_to_fg == nullptr)
    {
      cerr << "smash error: fg: job-id " << job_to_fg_id << " does not exist" << std::endl;
      return;
    }
  }
  else
  {
    cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
  // now to do the fg command
  // cout << "job_to_fg job id is: " << job_to_fg->job_id << endl;

  cout << job_to_fg->cmd_line << " : " << job_to_fg->job_pid << std::endl;

  int kill_res = kill(job_to_fg->job_pid, SIGCONT);
  if (kill_res == -1)
  {
    perror("smash error: kill failed");
    return;
  }
  // cout<<"after kill"<<endl;

  // ok

  job_to_fg->isForeground = true;
  job_to_fg->isBackground = false;
  job_to_fg->isStopped = false;
  // job_to_fg->isFinished = true;
  smash_inst.fg_job_id = job_to_fg->job_id;
  smash_inst.fg_job_pid = job_to_fg->job_pid;
  smash_inst.fg_job_cmd = job_to_fg->cmd_line;
  if (waitpid(job_to_fg->job_pid, NULL, WUNTRACED) == -1)
  {
    // cout<<"before waitpid"<<endl;
    perror("smash error: waitpid failed");
    return;
  }
  if (job_to_fg->isStopped == false)
  {
    job_to_fg->isFinished = true; ///////
    smash_inst.jobs_list->removeJobById(job_to_fg->job_id);
  }

  //  cout<<"after findNewMaxJobAndUpdate"<<endl;
  smash_inst.jobs_list->findNewMaxJobAndUpdate();
  smash_inst.fg_job_id = -1;
  smash_inst.fg_job_pid = -1;
  smash_inst.fg_job_cmd = "";
  return;
}
void BackgroundCommand::execute()
{
  JobsList::JobEntry *job;
  SmallShell &smash_inst = SmallShell::getInstance();
  if (smash_inst.num_args == 1)
  {
    JobsList::JobEntry *max_job = smash_inst.jobs_list->getLastStoppedJob();
    if (max_job == nullptr)
    {
      cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
      return;
    }
    job = max_job;
  }
  else if (smash_inst.num_args == 2)
  {
    int job_id = atoi(smash_inst.args[1]);
    if (job_id == 0)
    {
      cerr << "smash error: bg: invalid arguments" << std::endl;
      return;
    }
    JobsList::JobEntry *job_by_id = smash_inst.jobs_list->getJobById(job_id);
    if (job_by_id == nullptr)
    {
      // the id doesnt exist
      cerr << "smash error: bg: job-id " << (job_id) << " does not exist" << endl;
      return;
    }

    else if (job_by_id->isBackground && !job_by_id->isStopped)
    {
      cerr << "smash error: bg: job-id " << (job_id) << " is already running in the background" << endl;
      return;
    }
    job = job_by_id;
    // cout << "is background" << job_by_id->isBackground << "is stopped" << job_by_id->isStopped << endl;
  }
  else
  {
    cerr << "smash error: bg: invalid arguments" << endl;
    return;
  }
  // everything is ok
  //  print it
  cout << job->cmd_line << " : " << job->job_pid << endl;
  // sent the signal
  int kill_res = kill(job->job_pid, SIGCONT);
  if (kill_res != 0)
  {
    perror("smash error: kill failed");
    return;
  }
  job->isStopped = false;
  job->isBackground = true;
  job->isForeground = false;
  return;
}
void QuitCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  if (smash_inst.num_args != 2)
  { // ignore
    exit(0);
  }

  if (strcmp(smash_inst.args[1], "kill") == 0)
  {
    smash_inst.jobs_list->removeFinishedJobs();
    smash_inst.jobs_list->killAllJobs();
  }
  exit(0);
}

////////////////////////////joblist commands///////////////////////////////////
void JobsList::printJobsList()
{
  this->removeFinishedJobs();
  this->sortJobsList();
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    time_t *time_tlok = new time_t;
    time_t finish = time(time_tlok);
    time_t time_diff = difftime(finish, (*job_iterator)->start_time);
    cout << "[" << to_string((*job_iterator)->job_id) << "] " << (*job_iterator)->cmd_line << " : " << to_string((*job_iterator)->job_pid) << " " << to_string((long)(time_diff)) << " secs";
    if ((*job_iterator)->isStopped)
    {
      cout << " (stopped)";
    }
    cout << std::endl;
  }
}
JobsList::JobEntry *JobsList::getLastStoppedJob()
{
  for (auto iterator = jobs->rbegin(); iterator != jobs->rend(); iterator++)
  {

    if (((*iterator)->isStopped == true))
    {
      return (*iterator);
    }
  }
  return nullptr;
}
JobsList::JobEntry *JobsList::findNewMaxJobAndUpdate()
{
  int max_job_id = 0;
  JobsList::JobEntry *job_ptr = nullptr;
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    if ((*job_iterator)->job_id > max_job_id)
    {
      // if (((*job_iterator)->isStopped) || (*job_iterator)->isBackground)
      //{
      max_job_id = (*job_iterator)->job_id;
      job_ptr = *job_iterator;
      // }
    }
  }
  return job_ptr;
}
JobsList::JobEntry *JobsList::findMinJob()
{
  JobsList::JobEntry *job_ptr = nullptr;
  if (this->jobs->size() == 0)
  {
    // cout <<"null"<<endl;
    return nullptr;
  }
  int min_job_id = (*(this->jobs->begin()))->job_id;
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    if ((*job_iterator)->job_id <= min_job_id)
    {
      // if (((*job_iterator)->isStopped) || (*job_iterator)->isBackground)
      //{
      min_job_id = (*job_iterator)->job_id;
      job_ptr = *job_iterator;
      // }
    }
  }
  // cout << min_job_id<<endl;
  return job_ptr;
}
void JobsList::sortJobsList()
{
  std::vector<JobsList::JobEntry *> *new_sorted = new vector<JobsList::JobEntry *>();
  int size = this->jobs->size();
  if (size == 0)
  { // empty
    return;
  }
  int i = size;
  // cout<<"jobs size is "<<i<<endl;
  //  JobsList::JobEntry *min_job = nullptr;
  //  int min_id = this->jobs->begin()->job_id;
  while (i)
  {
    JobsList::JobEntry *min_job = this->findMinJob();
    if (min_job == nullptr)
    {
      break;
    }
    // cout << "curr min is "<< min_job->job_id<<endl;
    new_sorted->push_back(min_job);
    this->removeJobById(min_job->job_id);
    i--;
  }
  // cout << "printing sorted jobs id's"<<endl;
  // cout << "new sorted size is "<< new_sorted->size()<<endl;
  /*
  for (auto job_iterator = new_sorted->begin(); job_iterator != new_sorted->end(); ++job_iterator)
   {
    cout << (*(job_iterator))->job_id<<'\n';
   }
*/
  for (auto job_iterator = new_sorted->begin(); job_iterator != new_sorted->end(); ++job_iterator)
  {
    this->jobs->push_back(*(job_iterator));
  }
  delete new_sorted;
  // this->jobs = new_sorted;
  return;
}
JobsList::JobEntry *JobsList::getJobById(int job_id)
{
  JobsList::JobEntry *job_ptr = nullptr;
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    if ((*job_iterator)->job_id == job_id)
    {
      job_ptr = *job_iterator;
    }
  }
  return job_ptr;
}

JobsList::JobEntry *JobsList::getJobByPid(pid_t jobPid)
{
  JobsList::JobEntry *job_ptr = nullptr;
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    if ((*job_iterator)->job_pid == jobPid)
    {
      job_ptr = *job_iterator;
    }
  }

  return job_ptr;
}
int JobsList::numOfUnfinishedjobs()
{
  int count = 0;
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    if (!((*job_iterator)->isFinished))
    {
      count++;
    }
  }
  return count;
}
void JobsList::killAllJobs()
{
  cout << "smash: sending SIGKILL signal to " << this->numOfUnfinishedjobs() << " jobs:" << endl;
  vector<JobsList::JobEntry *>::iterator job_iterator;
  for (job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {

    cout << (*job_iterator)->job_pid << ": " << string((*job_iterator)->cmd_line) << endl;
    int kill_res = kill((*job_iterator)->job_pid, SIGKILL);
    if (kill_res == -1)
    {
      perror("smash error: kill failed");
      return;
    }
  }

  // this->removeFinishedJobs();
}
void JobsList::addJob(const char *cmd, pid_t pid, int job_id, bool isStopped)
{
  this->removeFinishedJobs();
  time_t *new_time = new time_t;
  int new_job_id;
  JobsList::JobEntry *job = findNewMaxJobAndUpdate();
  if (job == nullptr)
  {
    // cout<<"i am null"<<endl;
    new_job_id = 1;
  }
  else
  {
    new_job_id = job->job_id + 1;
  }
  if (job_id != -1)
  {
    new_job_id = job_id;
  }
  JobsList::JobEntry *new_job = new JobsList::JobEntry(new_job_id, pid, isStopped,
                                                       !isStopped, false, false, cmd, time(new_time));
  if (new_job != nullptr)
  {
    this->jobs->push_back(new_job);
  }
  return;
}

void JobsList::removeFinishedJobs()
{
  if (this->jobs->size() == 0)
  {
    return;
  }
  pid_t pid = waitpid(-1, NULL, WNOHANG);
  while (pid > 0)
  {
    for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
    {
      if ((*job_iterator)->job_pid == pid)
      {
        jobs->erase(job_iterator);
        findNewMaxJobAndUpdate();
        job_iterator = jobs->begin();
        break;
      }
    }
    pid = waitpid(-1, NULL, WNOHANG);
  }
}

void JobsList::removeJobById(int jobId)
{
  if (this->jobs->size() == 0)
  { // empty
    return;
  }
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    //  cout<<"i adwfewfewf"<<endl;

    if ((*job_iterator)->job_id == jobId)
    {
      this->jobs->erase(job_iterator);
      findNewMaxJobAndUpdate();
      return;
    }
  }
  /// cout<<"i am not fault"<<endl;
}

void JobsList::removeJobByPid(pid_t pid)
{
  if (this->jobs->size() == 0)
  { // empty
    return;
  }
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    //  cout<<"i adwfewfewf"<<endl;

    if ((*job_iterator)->job_pid == pid)
    {
      this->jobs->erase(job_iterator);
      findNewMaxJobAndUpdate();
      return;
    }
  }
  /// cout<<"i am not fault"<<endl;
}
JobsList::JobEntry *JobsList::getLastJob()
{
  JobsList::JobEntry *last_job;
  if (this->jobs->size() == 0)
  {
    last_job = nullptr;
  }
  for (auto job_iterator = this->jobs->begin(); job_iterator != this->jobs->end(); ++job_iterator)
  {
    last_job = (*job_iterator);
  }
  return last_job;
}

///////////////////////////////////external commands////////////////////////////
void ExternalCommand::execute()
{
  char c[C_FLAG_LEN] = "-c";
  char flags[BIN_LEN] = "/bin/bash";
  int wait_res;
  bool if_background = _isBackgroundComamnd(cmd_line.c_str());
  SmallShell &smash_inst = SmallShell::getInstance();
  size_t len = 1 + strlen(cmd_line.c_str());
  char cmd_line_for_bash[len];
  strcpy(cmd_line_for_bash, cmd_line.c_str());
  if (if_background)
  {
    _removeBackgroundSign(cmd_line_for_bash);
  }
  pid_t pid = fork();
  if (pid > 0) // dad
  {
    if (!if_background)
    {
      smash_inst.fg_job_pid = pid;
      smash_inst.fg_job_cmd = cmd_line_for_bash;
      wait_res = waitpid(pid, nullptr, WUNTRACED);
      if (wait_res == -1)
      {
        perror("smash error: waitpid failed");
        exit(0);
      }
      smash_inst.fg_job_id = -1;
      smash_inst.fg_job_pid = -1;
      smash_inst.fg_job_cmd = "";
    }
    else
    {
      smash_inst.jobs_list->addJob(cmd_line.c_str(), pid);
    }
    return;
  }
  else if (pid == 0)
  {
    setpgrp();

    char *argv[] = {flags, c, cmd_line_for_bash, NULL};
    if (execv(argv[0], argv) == -1)
    {
      perror("smash error: execv failed");
      exit(0);
    }
    exit(0);
  }

  else // if (pid == -1)
  {
    perror("smash error: fork failed");
    return;
  }
  return;
}

//////////////////////////////////special commands/////////////////////////////
//////////////////////////////////pipe command/////////////////////////////////
void PipeCommand::execute()
{
  string first_command;
  string second_command;
  int pipe_write_fd;
  SmallShell &smash_inst = SmallShell::getInstance();
  // int pipe_read_fd;
  unsigned int found_pos = ((string)cmd_line).find("|");
  if (found_pos != (string::npos))
  {
    // cout<<"fvdvwfa"<<endl;
    if (((string)cmd_line).find("|&") != (string::npos))
    {
      pipe_write_fd = 2; // stderr
      findPipeSecondAndFirstCommWithBG(cmd_line, found_pos, &first_command, &second_command);
    }
    else
    {
      pipe_write_fd = 1; // stout
      findPipeSecondAndFirstCommWithNoBg(cmd_line, found_pos, &first_command, &second_command);
    }
    // pipe_read_fd=0;//stdin
  }
  first_command = _trim(first_command);
  second_command = _trim(second_command);
  int my_pipe[PIPE_SIZE];
  if (pipe(my_pipe) == -1)
  {
    perror("smash error: pipe failed");
    return;
  }
  // first son
  int left_comm = fork();

  if (left_comm == -1)
  {
    perror("smash error: fork failed");
    close(my_pipe[0]);
    close(my_pipe[1]);
    return;
  }
  else if (left_comm == 0)
  {
    setpgrp();
    if (close(my_pipe[0]) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    int out_ = dup(pipe_write_fd);
    if (out_ == -1)
    {
      perror("smash error: dup failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    if (dup2(my_pipe[PIPE_WRITE], pipe_write_fd) == -1)
    {
      perror("smash error: dup2 failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    smash_inst.executeCommand(first_command.c_str());
    if (close(pipe_write_fd) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    if (dup2(out_, pipe_write_fd) == -1)
    {
      perror("smash error: dup2 failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    if (close(out_) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    if (close(my_pipe[1]) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[1]);
      return;
    }
    exit(0);
  }
  else
  {
    if (close(my_pipe[1]) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[1]);
      close(my_pipe[0]);
      return;
    }
    int in_ = dup(0);
    if (in_ == -1)
    {
      perror("smash error: dup failed");
      close(my_pipe[0]);
      return;
    }
    if (dup2(my_pipe[0], 0) == -1)
    {
      perror("smash error: dup2 failed");
      close(my_pipe[0]);
      return;
    }
    if (waitpid(left_comm, NULL, 0) == -1)
    {
      perror("smash error: waitpid failed");
      close(my_pipe[0]);
      return;
    }
    smash_inst.executeCommand(second_command.c_str());
    if (close(0) == -1)
    {
      perror("smash error: close failed");
      close(my_pipe[0]);
      return;
    }
    if (dup2(in_, 0) == -1)
    {
      perror("smash error: dup2 failed");
      close(my_pipe[0]);
      return;
    }
    int close_1 = close(my_pipe[0]);
    int close_2 = close(in_);
    if (close_1 == -1 || close_2 == -1)
    {
      perror("smash error: close failed");
      return;
    }
  }
  return;
}
//////////////////////////////redirection command//////////////////////////////
void RedirectionCommand::execute()
{

  SmallShell &smash_inst = SmallShell::getInstance();
  string output_file;
  string the_command;
  int open_out_fd;
  unsigned int found_file_pos = (string(cmd_line)).find(">");
  // returns the index of the first occurrence of the substring in the string ">>"
  if (found_file_pos != (string::npos))
  {
    // it is >>
    if (((string)cmd_line).find(">>") != (string::npos))
    {

      findPipeSecondAndFirstCommWithDoubleRedirection(cmd_line, found_file_pos, &the_command, &output_file);
      output_file = _trim(output_file);
      open_out_fd = open(output_file.c_str(), O_CREAT | O_RDWR | O_APPEND, 0655);

      if ((open_out_fd == -1))
      {
        perror("smash error: open failed");
        return;
      }
    }
    else
    {
      found_file_pos = (string(cmd_line)).find(">");
      findPipeSecondAndFirstCommWithNoBg(cmd_line, found_file_pos, &the_command, &output_file);
      output_file = _trim(output_file);
      open_out_fd = open(output_file.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0655);
      if ((open_out_fd == -1))
      {
        perror("smash error: open failed");
        return;
      }
    }
  }

  int ptr_to_screen = dup(1);

  if ((ptr_to_screen == -1))
  {
    perror("smash error: dup failed");
    return;
  }
  int dup2_res = dup2(open_out_fd, 1);

  if ((dup2_res == -1))
  {
    perror("smash error: dup2 failed");
    return;
  }
  Command *comm = smash_inst.CreateCommand(the_command.c_str());
  comm->execute();
  if (close(1) == -1)
  {
    perror("smash error: close failed");
    delete comm;
    return;
  }
  int res = dup2(ptr_to_screen, 1);
  if ((res == -1))
  {
    perror("smash error: dup2 failed");
    delete comm;
    return;
  }
  if (close(ptr_to_screen) == -1)
  {
    perror("smash error: close failed");
    delete comm;
    return;
  }

  if (close(open_out_fd) == -1)
  {
    perror("smash error: close failed");
    delete comm;
    return;
  }
  return;
}

////////////////////////////////////tail command////////////////////////////////
static int countLines(int fd_open_res, int *flag)
{
  char *buffer = new char[1];
  int lines_num = 0;
  int read_res;
  // int i = 0;
  while ((read_res = read(fd_open_res, buffer, 1)) > 0)
  {
    // cout << "i" << i << endl;
    if ('\n' == buffer[0])
    {
      lines_num++;
    }
  }

  if (read_res == -1)
  {
    delete buffer;
    perror("smash error: read failed");
    return -1;
  }
  // cout << "11" << endl;
  if (lines_num > 0)
  {
    int lseek_res = lseek(fd_open_res, -1, SEEK_END);
    // cout << "11" << endl;

    if (lseek_res == -1)
    {
      // cout << "21" << endl;
      delete buffer;
      perror("smash error: lseek failed");
      return -1;
    }
    else
    {
      int resss = read(fd_open_res, buffer, 1);
      if (resss == 1)
      {
        if ('\n' == buffer[0])
        {
          *flag = 1;
          // lines_num--;
        }
        else
        {
          *flag = 0;
          lines_num++;
        }
      }
    }
  }

  return lines_num;
}
void TailCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  int N = 0;
  char *file_name = nullptr;
  bool is_3_args = false;
  int ended_with_new_line = 0;
  string new_line = "\n";
  string zero_str = "-0";
  if (((smash_inst.num_args != 2) && (smash_inst.num_args != 3)) ||
      ((smash_inst.num_args == 3) && (atoi(smash_inst.args[1]) > 0)) ||
      ((smash_inst.num_args == 3) && ((atoi(smash_inst.args[1])) == 0) &&
       (!(is_number(string(smash_inst.args[1])))) && ((strcmp((zero_str).c_str(), smash_inst.args[1])))))
  {
    cerr << "smash error: tail: invalid arguments" << endl;
    return;
  }
  if (smash_inst.num_args == 2)
  {
    N = 10;
  }
  else if (smash_inst.num_args == 3)
  {
    N = atoi(smash_inst.args[1]) * (-1);
    is_3_args = true;
  }
  if (N == 0)
  {
    return;
  }
  if (is_3_args)
    file_name = smash_inst.args[2];
  else
    file_name = smash_inst.args[1];

  char *buffer = new char[1];
  int fd_open_res = open(file_name, O_RDONLY);

  if (fd_open_res == -1)
  {
    perror("smash error: open failed");
    return;
  }

  int lines_num = countLines(fd_open_res, &ended_with_new_line);
  // cout << "lines num" << lines_num << endl;

  if (lines_num == -1)
    return;
  int lseek_res = lseek(fd_open_res, 0, SEEK_SET);

  if (lseek_res == -1)
  {
    // cout << "33" << endl;
    delete buffer;
    perror("smash error: lseek failed");
    return;
  }

  int iteration_num = lines_num - N + 1;
  // cout << "iteration_num" << iteration_num << endl;

  if (N >= lines_num)
  {
    // cout << "N" << N << endl;
    iteration_num = 0;
  }
  int i = 1, read_res;
  // cout << "after iteration_num" << iteration_num << endl

  while (i < iteration_num)
  {

    // cout << "I'm here at"<<" 1st while"<<endl;
    read_res = read(fd_open_res, buffer, 1);
    //     if (smash_inst.num_args == 2)
    //     {
    // //string s = buffer;
    // //cout<<"buffer  "<<buffer[0]<<"  i  "<<i<<endl;
    //     }

    if (read_res == -1)
    {

      delete buffer;
      perror("smash error: read failed");
      return;
    }
    else if (read_res == 1) // not eof
    {

      // file_content_str = file_content_str + buffer[0];
      char line = '\n';

      if (line == buffer[0])
      {
        i++;
      }
    }
    else if (read_res == 0)
    {
      break;
    }
  }

  ssize_t write_res;
  // int j = 0;
  int res;
  // cout<<"res"<<res<<endl;
  while ((res = read(fd_open_res, buffer, 1)) > 0)
  {
    // cout << "efed" << endl;

    write_res = write(1, buffer, 1);
    // j++;
    if (write_res == -1)
    {
      delete buffer;
      perror("smash error: write failed");
      return;
    }
    else if (write_res == 0)
    {
      break;
    }
    if (('\n' == buffer[0]) &&
        (checkNextEofLine(fd_open_res)) && ended_with_new_line == 1)
    {
      break;
    }
  }
  if (read_res == 0)
  {
    delete buffer;

    // cout<<"read is zero"<<endl;
    return;
  }
  else if (read_res == -1)
  {
    //   cout<<"read is -1"<<endl;

    delete buffer;
    perror("smash error: read failed");
    return;
  }
  delete buffer;
  return;
}

static bool checkNextEofLine(int fd_open_res)
{
  int read_res;
  char *buff = new char[1];
  if ((read_res = read(fd_open_res, buff, 1)) == 0)
  {
    return true;
  }
  else
  {
    int lseek_res = lseek(fd_open_res, -1, SEEK_CUR);

    if (lseek_res == -1)
    {
      delete buff;
      perror("smash error: lseek failed");
      return false;
    }
    return false;
  }
}
////////////////////////////////touch command///////////////////////////////////
void TouchCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  if (smash_inst.num_args != 3)
  {
    cerr << "smash error: touch: invalid arguments" << endl;
    return;
  }

  char time_stamp_char_array[21] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
  strcpy(time_stamp_char_array, smash_inst.args[2]);

  int i = 0;
  char tmp[5];
  int array_time[6];
  int curr_time_field = 0;
  while (i < 21 && curr_time_field < 6)
  {
    int digits_num = 0;
    while (i < 21 && time_stamp_char_array[i] != ':')
    {
      tmp[digits_num] = time_stamp_char_array[i];
      digits_num++;
      i++;
    }
    tmp[digits_num] = '\0';
    array_time[curr_time_field] = atoi(tmp);
    i++;
    curr_time_field++;
  }

  struct tm input_for_mktime;
  input_for_mktime.tm_sec = array_time[0];
  input_for_mktime.tm_min = array_time[1];
  input_for_mktime.tm_hour = array_time[2];
  input_for_mktime.tm_mday = array_time[3];
  input_for_mktime.tm_mon = array_time[4] - 1;
  input_for_mktime.tm_year = array_time[5] - 1900;
  input_for_mktime.tm_isdst = -1;

  time_t mktime_res = mktime(&input_for_mktime);
  if (mktime_res == -1)
  {
    perror("smash error: mktime failed");
    return;
  }

  struct utimbuf utime_input;
  utime_input.actime = mktime_res;
  utime_input.modtime = mktime_res;
  int utime_res = utime(smash_inst.args[1], &utime_input);
  if (utime_res == -1)
  {
    perror("smash error: utime failed");
    return;
  }

  return;
}
static bool is_number(const string &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}
//////////////////////////////////////timeout command list functions////////////////////////////////////////////
void TimeOutListClass::insertTimeOutCommand(int to_duration, pid_t to_pid, string to_cmd_line)
{
  time_t *time_tlok = new time_t;
  time_t to_command_time = time(time_tlok);
  TimeOutListClass::TimeOutCommandClass *command_to_add =
      new TimeOutListClass::TimeOutCommandClass(to_command_time, to_duration, to_pid, to_cmd_line);
  this->to_commands_vec->push_back(command_to_add);
  this->alarmForShortestDuration();
}
void TimeOutListClass::deleteTimedOutCommand(pid_t pid)
{
  for (auto to_iterator = this->to_commands_vec->begin(); to_iterator != this->to_commands_vec->end(); ++to_iterator)
  {
    bool found_pid = ((*to_iterator)->to_command_pid == pid);
    if (found_pid)
    {
      this->to_commands_vec->erase(to_iterator);
      return;
    }
  }
}
void TimeOutListClass::alarmForShortestDuration()
{
  if (this->to_commands_vec->size() == 0)
  {
    return;
  }
  time_t *time_tlok = new time_t;
  time_t curr_time = time(time_tlok);
  double min = (*(to_commands_vec->begin()))->to_command_duration - difftime(curr_time, (*(to_commands_vec->begin()))->to_command_timestamp);

  for (auto iterator = to_commands_vec->begin(); iterator != to_commands_vec->end(); iterator++)
  {
    double tmp = (*iterator)->to_command_duration - difftime(curr_time, (*(iterator))->to_command_timestamp);
    if (min > tmp)
    {
      min = tmp;
    }
  }
  alarm(min);
}

////////////////////////////////////////////Timeout Command///////////////////////////////////////////////
void TimeoutCommand::execute()
{
  SmallShell &smash_inst = SmallShell::getInstance();
  if (smash_inst.num_args < 3)
  {
    cerr << "smash error: timeout: invalid arguments" << endl;
    return;
  }

  int duration = atoi(smash_inst.args[1]);
  if (duration <= 0)
  {
    cerr << "smash error: timeout: invalid arguments" << endl;
    return;
  }

  size_t timed_command_pos = (string(cmd_line)).find(smash_inst.args[1]) + 1;
  int timed_command_size = cmd_line.size() - timed_command_pos - 1;
  string timed_command_str = cmd_line.substr(timed_command_pos + 1, timed_command_size);
  timed_command_str = _trim(timed_command_str);

  bool is_background = _isBackgroundComamnd(timed_command_str.c_str());
  char *cmd_line_for_bash = new char[timed_command_str.size() + 1];
  strcpy(cmd_line_for_bash, timed_command_str.c_str());
  if (is_background)
  {
    _removeBackgroundSign(cmd_line_for_bash);
  }

  pid_t pid = fork();
  if (pid == 0)
  {
    setpgrp();
    // cout << "the cmd command is: "<<cmd_line_for_bash<<endl;
    int res_execl = execl("/bin/bash", "bash", "-c", cmd_line_for_bash, nullptr);
    if (res_execl == -1)
    {
      perror("smash error: execv failed");
      exit(0);
    }
    exit(0);
  }
  else if (pid > 0) // dad
  {
    if (!is_background)
    {
      smash_inst.fg_job_pid = pid;
      smash_inst.fg_job_cmd = cmd_line_for_bash;
      smash_inst.timeout_list->insertTimeOutCommand(duration, pid, timed_command_str);
      // it is not background
      int wait_res = waitpid(pid, nullptr, WUNTRACED);
      if (wait_res == -1)
      {
        perror("smash error: waitpid failed");
        return;
      }
      smash_inst.fg_job_id = -1;
      smash_inst.fg_job_pid = -1;
    }
    else
    {
      smash_inst.jobs_list->addJob(cmd_line.c_str(), pid);
      // cout << "before insert" << endl;
      smash_inst.timeout_list->insertTimeOutCommand(duration, pid, timed_command_str);
      // cout <<"after insert" << endl;
    }
    return;
  }
  else // if (pid == -1)
  {
    perror("smash error: fork failed");
    return;
  }

  return;
}

///////static functions AUX

static void findPipeSecondAndFirstCommWithBG(string cmd_line, unsigned int found_pos, string *first_command, string *second_command)
{

  *first_command = cmd_line.substr(0, found_pos);
   unsigned int pos = (string(cmd_line)).find("&");
  size_t command_2_size = cmd_line.size() - pos-1;
  *second_command = cmd_line.substr(pos + 1, command_2_size);
}
static void findPipeSecondAndFirstCommWithDoubleRedirection(string cmd_line, unsigned int found_pos, string *first_command, string *second_command)
{
  *first_command = cmd_line.substr(0, found_pos);
  size_t command_2_size = cmd_line.size() - found_pos-2;
  *second_command = cmd_line.substr(found_pos + 2, command_2_size);
}
static void findPipeSecondAndFirstCommWithNoBg(string cmd_line, unsigned int found_pos, string *first_command, string *second_command)
{
  *first_command = cmd_line.substr(0, found_pos);
  int command_2_len = (cmd_line.size()) - found_pos - 1;
  *second_command = cmd_line.substr(found_pos + 1, command_2_len);
}
