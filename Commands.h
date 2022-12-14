#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <memory>
using std::string;
using std::shared_ptr;
using std::vector;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

#define DO_SYS( syscall, name ) do { \
    if( (syscall) == -1 ) { \
      perror("smash error: " #name " failed"); \
      return; \
    }         \
  } while(0)  \

enum JobStatus {BACKGROUND, FOREGROUND, STOPPED};
class Command {
// TODO: Add your data members
protected:
    string cmd_line;
 public:
  Command(const char* cmd_line) : cmd_line(cmd_line) {}
  virtual ~Command() = default;
  virtual void execute() = 0;
  string getCmdLine();
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line) : Command(cmd_line){}
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
  public:
  ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

//finished
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

//finished
class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
    JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs(jobs) {}
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
  public:
      int pid;
      int job_id;
      JobStatus status;
      string cmd_line;
      time_t time_added;


      JobEntry(int pid , int job_id , JobStatus status , string cmd_line , time_t time_added)
      : pid(pid) , job_id(job_id) , status(status) , cmd_line(cmd_line), time_added(time_added) {}
  };
 // TODO: Add your data members
  vector<shared_ptr<JobEntry>> jobs;
 public:
  JobsList() = default;
  ~JobsList() = default;
  void addJob(Command *cmd, int pid, JobStatus status);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
  JobsList* jobs;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs(jobs) {}
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
  JobsList* jobs;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs(jobs){}
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutList {
public:
    class TimeoutEntry {
    public:
        int pid;
        time_t time_added;
        time_t duration;
        string cmd_line;
        TimeoutEntry(int pid, time_t time_added, time_t duration, string cmd_line) : pid(pid), time_added(time_added),
        duration(duration), cmd_line(cmd_line){}
        ~TimeoutEntry() = default;
    };
    vector<shared_ptr<TimeoutEntry>> commands;
    TimeoutList() = default;
    ~TimeoutList() = default;
    void addTimeoutCommand(int pid, time_t duration, string cmd_line);
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
 // TODO: Add your data members
  JobsList* jobs;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs(jobs) {}
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  // TODO: Add your data members
  string shell_prompt;
  int shell_pid;
  string prev_dir;
  JobsList* jobs;

  SmallShell();
 public:
    int running_id;
    int running_pid;
    string running_cmd;
    TimeoutList* timeouts;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed

    //for function <chprompt>
    void setShellPrompt(string newShellPrompt);
    string getShellPrompt();

    //for function <showpid>
    int getShellPid();

    //for function <cd>
    string getPrevDir()
    {
        return prev_dir;
    }
    void setPrevDir(string new_dir)
    {
        prev_dir = new_dir;
    }

    //for externals
    JobsList* getJobsList()
    {
        return jobs;
    }
};

#endif //SMASH_COMMAND_H_
