#ifndef COMMANDS_H_
#define COMMANDS_H_
using std::string;
#include <vector>
#include <string>
#define COMMAND_ARGS_MAX_LENGTH (80)
#define COMMAND_MAX_ARGS (20)
using std::vector;

class Command
{
public:
  // TODO: Add your data members
  string cmd_line;

public:
  Command(const char *cmd_line) : cmd_line((string)cmd_line){};
  virtual ~Command() = default;
  virtual void execute() = 0;
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command
{
public:
  BuiltInCommand(const char *cmd_line) : Command(cmd_line){};
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command
{
public:
  ExternalCommand(const char *cmd_line) : Command(cmd_line){};
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command
{
  // TODO: Add your data members
public:
  PipeCommand(const char *cmd_line) : Command(cmd_line){};
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command
{
  // TODO: Add your data members
public:
  RedirectionCommand(const char *cmd_line) : Command(cmd_line){};
  virtual ~RedirectionCommand() {}
  void execute() override;
  // void prepare() override;
  // void cleanup() override;
};
// cd
class ChangeDirCommand : public BuiltInCommand
{
public:
  // TODO: Add your data members public:
  ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~ChangeDirCommand() {}
  void execute() override;
};
// pwd
class GetCurrDirCommand : public BuiltInCommand
{
public:
  GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};
// showpid
class ShowPidCommand : public BuiltInCommand
{
public:
  ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~ShowPidCommand() {}
  void execute() override;
};
// chprompt
class ChpromptCommand : public BuiltInCommand
{
public:
  ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~ChpromptCommand() {}
  void execute() override;
};
class JobsList;
class QuitCommand : public BuiltInCommand
{
public:
  // TODO: Add your data members public:
  QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~QuitCommand() {}
  void execute() override;
};
class JobsList
{
public:
  class JobEntry
  {
  public:
    // TODO: Add your data members
    int job_id;
    pid_t job_pid;
    bool isStopped;
    bool isBackground;
    bool isForeground; ////
    bool isFinished;
    string cmd_line;
    time_t start_time;
    /// member functions
    JobEntry() = default;
    JobEntry(int job_id, pid_t job_pid, bool isStopped, bool isBackground, bool isForeground, bool isFinished, string cmd_line, time_t start_time_) : job_id(job_id), job_pid(job_pid), isStopped(isStopped), isBackground(isBackground), isForeground(isForeground), isFinished(isFinished), cmd_line(cmd_line),
                                                                                                                                                      start_time(start_time_) {}
    // friend bool operator>(const JobEntry &, const JobEntry &);
    // friend bool operator==(const JobEntry &, const JobEntry &);
    // frined bool operator<(const JobEntry &, const JobEntry &);
  };
  // TODO: Add your data members
  std::vector<JobsList::JobEntry *> *jobs;

public:
  JobsList() : jobs(new vector<JobsList::JobEntry *>()) {}
  ~JobsList() = default;
  ///////the destructor is default???????
  void addJob(const char *cmd, pid_t pid, int job_id = -1, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  // void removeFinishedJobs_(pid_t pid);
  JobEntry *getJobById(int jobId);
  JobEntry *getJobByPid(pid_t jobPid);
  void removeJobByPid(pid_t pid);
  void removeJobById(int jobId);
  JobEntry *getLastJob();
  JobEntry *getLastStoppedJob();
  // TODO: Add extra methods or modify exisitng ones as needed
  JobEntry *findNewMaxJobAndUpdate();
  JobEntry *findMinJob();
  void sortJobsList();
  int numOfUnfinishedjobs();
};
// bool operator>(const JobEntry &, const JobEntry &);
// bool operator==(const JobEntry &, const JobEntry &);
// bool operator<(const JobEntry &, const JobEntry &);
class JobsCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TailCommand : public BuiltInCommand
{
public:
  TailCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~TailCommand() {}
  void execute() override;
};

class TouchCommand : public BuiltInCommand
{
public:
  TouchCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
  virtual ~TouchCommand() {}
  void execute() override;
};

class TimeoutCommand : public Command
{
  int duration;

public:
  TimeoutCommand(const char *cmd_line) : Command(cmd_line){};
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class TimeOutListClass
{
public:
  class TimeOutCommandClass
  {
  public:
    time_t to_command_timestamp;
    int to_command_duration;
    pid_t to_command_pid;
    string to_command_cmd_line;
    TimeOutCommandClass(time_t to_command_timestamp, int to_command_duration, pid_t to_command_pid, string to_command_cmd_line) : to_command_timestamp(to_command_timestamp), to_command_duration(to_command_duration),
                                                                                                                                  to_command_pid(to_command_pid), to_command_cmd_line(to_command_cmd_line)
    {
    }
    TimeOutCommandClass() = default;
  };
  std::vector<TimeOutListClass::TimeOutCommandClass *> *to_commands_vec;

public:
  TimeOutListClass() : to_commands_vec(new std::vector<TimeOutListClass::TimeOutCommandClass *>()) {}
  ~TimeOutListClass() {}
  void insertTimeOutCommand(int to_duration, pid_t to_pid, string to_cmd_line);
  void deleteTimedOutCommand(pid_t pid_to_del);
  void alarmForShortestDuration();
};
class SmallShell
{
public:
  // TODO: Add your data members
  pid_t smash_pid;
  JobsList *jobs_list;
  // JobsList *FG_jobs;
  string curr_prompt;
  string last_updated_directory;
  int fg_job_id;
  pid_t fg_job_pid;
  string fg_job_cmd;
  //bool quit;
  int num_args;
  char **args;
  TimeOutListClass *timeout_list;

  SmallShell();

public:
  Command *CreateCommand(const char *cmd_line);
  SmallShell(SmallShell const &) = delete;     // disable copy ctor
  void operator=(SmallShell const &) = delete; // disable = operator
  static SmallShell &getInstance()             // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell() = default;
  void executeCommand(const char *cmd_line);
  // TODO: add extra methods as needed
};

#endif // SMASH_COMMAND_H_
