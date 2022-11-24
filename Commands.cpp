#include <unistd.h>
#include <string.h>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <limits.h>
#include "Commands.h"
#include <signal.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}


//*** Shell class **************
void SmallShell::setShellPrompt(string newShellPrompt = "smash")
{
    this->shell_prompt = newShellPrompt;
}
string SmallShell::getShellPrompt()
{
    return this->shell_prompt;
}

int SmallShell::getShellPid()
{
    return this->shell_pid;
}

// ************* PWD class *****************
void GetCurrDirCommand::execute()
{
    char cwd[PATH_MAX];
    if(getcwd(cwd , sizeof (cwd)) != NULL)
    {
        cout << cwd << endl;
    }
    else
    {
        perror("smash error: getcwd failed");
    }
}

//*********** show Pid class ******
void ShowPidCommand::execute()
{
    cout << "smash pid is " << SmallShell::getInstance().getShellPid() << endl;
}

//*********** chdir class **********
void ChangeDirCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    string new_dir;
    string command = this->cmd_line;
    command = _trim(command);
    // if (cd) with no parameters
    if ((int)command.length() == 3)
    {
        cerr << "smash error:>\"cd\"";
        return;
    }
    string params_only = command.substr(command.find_first_of(" \n"));
    params_only = _trim(params_only);
    string first_param = params_only.substr(0 , params_only.find_first_of(" \n"));
    //if more than 1 params
    if(params_only.find_first_of(" \n") != string::npos)
    {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if(first_param == "-")
    {
        if("" == shell.getPrevDir())
        {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        else
        {
            new_dir = shell.getPrevDir();
        }
    }
    else
    {
        new_dir = first_param;
    }
    char curr_dir[PATH_MAX];
    if(getcwd(curr_dir , sizeof(curr_dir)) == NULL)
    {
        perror("smash error: getcwd failed");
    }
    int sys_ret;
    DO_SYS(sys_ret = chdir(new_dir.c_str()) , chdir);
    if(sys_ret != -1)
    {
        shell.setPrevDir(curr_dir);
    }
}

//*************** Command ***********
string Command::getCmdLine()
{
    return this->cmd_line;
}

//***************************Jobs*********
void JobsList::addJob(Command* cmd, bool isStopped)
{
    removeFinishedJobs();
    int new_id = 0;
    int pid;
    for(int i = 0 ; i < (int)this->jobs.size() ; ++i)
    {
        if(this->jobs[i]->job_id > new_id)
        {
            new_id = this->jobs[i]->job_id;
        }
    }
    ++new_id;

    time_t now;
    if (time(&now) == ((time_t) -1) ) {
        perror("smash error: time failed");
        return;
    }
    DO_SYS(pid = getpid(), getpid); // where to put pid
    this->jobs.push_back(make_shared<JobEntry>(pid, new_id, 0,cmd->getCmdLine(), now));
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (int i = 0; i < jobs.size(); ++i) {
        time_t now;
        if (time(&now) == ((time_t) -1) ) {
            perror("smash error: time failed");
            return;
        }
        time_t difference = difftime(now, jobs[i]->time_added);
        cout << "[" << jobs[i]->job_id << "] " << jobs[i]->cmd_line <<
                    " : " << jobs[i]->pid << " " << difference << " secs ";

        if (jobs[i]->is_stopped) {
            cout << "(stopped)";
        }

        cout << endl;
    }
}

void JobsList::killAllJobs() {
    cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << endl;
    for (int i = 0; i < jobs.size(); ++i) {
        cout << jobs[i]->pid << ": " << jobs[i]->cmd_line << endl;
        DO_SYS(kill(jobs[i]->pid, SIGKILL), kill);
    }
}

void JobsList::removeFinishedJobs() {
    vector<shared_ptr<JobEntry>> nonFinished;
    for (int i = 0; i < jobs.size(); ++i) {
        if (waitpid(jobs[i]->pid, NULL, WNOHANG) <= 0) {
            nonFinished.push_back(jobs[i]);
        }
    }

    this->jobs = nonFinished;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (int i = 0; i < jobs.size(); ++i) {
        if (jobs[i]->job_id == jobId) {
            return jobs[i].get();
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
//    shared_ptr<>
    vector<shared_ptr<JobEntry>>::iterator toDelete;
    for (int i = 0; i < jobs.size(); ++i) {
        if(this->jobs[i]->job_id == jobId)
        {
            this->jobs.erase(toDelete);
        }
        ++toDelete;
    }
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId) {
    return jobs[jobs.size() - 1].get();
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    for (int i = jobs.size() - 1; i >= 0; --i) {
        if (jobs[i]->is_stopped) {
            return jobs[i].get();
        }
    }
    return nullptr;
}

void ForegroundCommand::execute() {

}
