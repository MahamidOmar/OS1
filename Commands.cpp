
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
#include <csignal>
#include <errno.h>
#include <fcntl.h>
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

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
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

SmallShell::SmallShell() : shell_prompt("smash"), prev_dir(""), jobs(new JobsList()) , running_id(-1) , running_pid(-1)
                    ,running_cmd(""), timeouts(new TimeoutList()){
    DO_SYS(shell_pid = getpid(), getpid);
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete jobs;
    delete timeouts;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string trimmed = _trim(cmd_line);
    string command = trimmed.substr(0, trimmed.find_first_of(" \n"));
    if (trimmed.find('>') != string::npos)
    {
        return new RedirectionCommand(cmd_line);
    }
    if (trimmed.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    if (trimmed == "")
    {
        return nullptr;
    }
    if (command == "quit") {
        return new QuitCommand(cmd_line, jobs);
    }
    if (command == "chprompt") {
        string first_param;
        if (trimmed.find_first_of(" \n") == string::npos) {
            first_param = "smash";
        } else {
            string params_only = _trim(trimmed.substr(trimmed.find_first_of(" \n")));
            first_param = params_only.substr(0, params_only.find_first_of(" \n"));
        }
        setShellPrompt(first_param);
        return nullptr;
    }
    if (command == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    if (command == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    if (command == "cd") {
        return new ChangeDirCommand(cmd_line);
    }
    if (command == "jobs") {
        return new JobsCommand(cmd_line, jobs);
    }
    if (command == "kill") {
        return new KillCommand(cmd_line, jobs);
    }
    if (command == "bg") {
        return new BackgroundCommand(cmd_line, jobs);
    }
    if (command == "fg") {
        return new ForegroundCommand(cmd_line, jobs);
    }

    return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs->removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if (cmd)
    {
        cmd->execute();
    }

}


//*** Shell class **************
void SmallShell::setShellPrompt(string newShellPrompt = "smash") {
    this->shell_prompt = newShellPrompt;
}

string SmallShell::getShellPrompt() {
    return this->shell_prompt;
}

int SmallShell::getShellPid() {
    return this->shell_pid;
}

// ************* PWD class *****************
void GetCurrDirCommand::execute() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        cout << cwd << endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

//*********** show Pid class ******
void ShowPidCommand::execute() {
    cout << "smash pid is " << SmallShell::getInstance().getShellPid() << endl;
}

//*********** chdir class **********
void ChangeDirCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    string new_dir;
    string command = this->cmd_line;
    command = _trim(command);
    // if (cd) with no parameters
    if ((int) command.length() == 3) {
        cerr << "smash error:>\"cd\"";
        return;
    }
    string params_only = command.substr(command.find_first_of(" \n"));
    params_only = _trim(params_only);
    string first_param = params_only.substr(0, params_only.find_first_of(" \n"));
    //if more than 1 params
    if (params_only.find_first_of(" \n") != string::npos) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (first_param == "-") {
        if ("" == shell.getPrevDir()) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            new_dir = shell.getPrevDir();
        }
    } else {
        new_dir = first_param;
    }
    char curr_dir[PATH_MAX];
    if (getcwd(curr_dir, sizeof(curr_dir)) == NULL) {
        perror("smash error: getcwd failed");
    }
    int sys_ret;
    DO_SYS(sys_ret = chdir(new_dir.c_str()), chdir);
    if (sys_ret != -1) {
        shell.setPrevDir(curr_dir);
    }
}

//*************** Command ***********
string Command::getCmdLine() {
    return this->cmd_line;
}

//***************************Jobs*********
void JobsList::addJob(Command *cmd, int pid, JobStatus status) {
    removeFinishedJobs();
    int new_id = 0;
    int index = -1;
    for (int i = 0; i < (int) this->jobs.size(); ++i) {
        if (this->jobs[i]->job_id > new_id) {
            new_id = this->jobs[i]->job_id;
        }
        if (jobs[i]->pid == pid) {
            index = i;
        }
    }
    ++new_id;
    time_t now;
    if (time(&now) == ((time_t) -1)) {
        perror("smash error: time failed");
        return;
    }
    if (index != -1) {
        jobs[index]->time_added = now;
        jobs[index]->status = status;
        return;
    }

    this->jobs.push_back(make_shared<JobEntry>(pid, new_id, status, cmd->getCmdLine(), now));
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for (int i = 0; i < jobs.size(); ++i) {
        time_t now;
        if (time(&now) == ((time_t) -1)) {
            perror("smash error: time failed");
            return;
        }
        if (jobs[i]->status == FOREGROUND) {
            continue;
        }
        time_t difference = difftime(now, jobs[i]->time_added);
        cout << "[" << jobs[i]->job_id << "] " << jobs[i]->cmd_line <<
             " : " << jobs[i]->pid << " " << difference << " secs";

        if (jobs[i]->status == STOPPED) {
            cout << " (stopped)";
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
        int sys_ret = 0;
        sys_ret = waitpid(jobs[i]->pid, NULL, WNOHANG);
        if (sys_ret == 0) {
            nonFinished.push_back(jobs[i]);
        }
//        cout << sys_ret << "  job pid = "<<jobs[i]->pid << endl;
    }

    this->jobs = nonFinished;
//    this->jobs.erase(remove_if(jobs.begin() , jobs.end() , [](shared_ptr<JobsList::JobEntry> element)
//    {
//        int pid = element->pid;
//        int flag;
//        flag = waitpid(pid , NULL , WNOHANG);
//        return flag > 0;
//    }
//    ) , jobs.end());
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
        if (this->jobs[i]->job_id == jobId) {
            jobs[i]->status = FOREGROUND;
        }
    }
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    if (jobs.size() == 0) {
        *lastJobId = 0;
        return nullptr;
    }
    *lastJobId = jobs.size() - 1;
    return jobs[jobs.size() - 1].get();
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    for (int i = jobs.size() - 1; i >= 0; --i) {
        if (jobs[i]->status == STOPPED) {
            *jobId = jobs[jobs.size() - 1]->job_id;
            return jobs[i].get();
        }
    }
    *jobId = 0;
    return nullptr;
}

void JobsCommand::execute() {
    jobs->printJobsList();
}
//********** ForeGround Command ***************
void ForegroundCommand::execute() {
    string command = _trim(this->cmd_line);
    string params_only = "";
    string first_param = "";
    bool bad_params = false;
    SmallShell &shell = SmallShell::getInstance();
    if (command.find_first_of(" \n") != string::npos) {
        params_only = _trim(command.substr(command.find_first_of(" \n")));
        first_param = params_only.substr(0, params_only.find_first_of(" \n"));
    }
    int job_id;
    if (first_param == "") {
        jobs->getLastJob(&job_id);
        if (job_id == 0) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    } else
    {
        try
        {
            job_id = stoi(first_param);
        } catch (std::exception &e)
        {
            bad_params = true;
        }
        bad_params = bad_params || (params_only.find_first_of(" \n") != string::npos);
        if (bad_params)
        {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
    }

    JobsList::JobEntry *job = jobs->getJobById(job_id);
    if (!job) {
        cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    //check if the job is stopped, send sigcont
    if (job->status == STOPPED) {
        DO_SYS(kill(job->pid, SIGCONT), kill);
    }
    job->status = FOREGROUND;
    shell.running_cmd = job->cmd_line;
    shell.running_pid = job->pid;
    shell.running_id = job->job_id;
    cout << job->cmd_line << " : " << job->pid << endl;
    jobs->removeJobById(job_id); // change later
    DO_SYS(waitpid(job->pid, NULL, WUNTRACED), waitpid);
    shell.running_pid = -1;
    shell.running_id = -1;
    shell.running_cmd = "";
}

//********** BackGround Command ***************
void BackgroundCommand::execute() {
    string command = _trim(this->cmd_line);
    string params_only = "";
    string first_param = "";
    bool bad_params = false;
    SmallShell &shell = SmallShell::getInstance();
    if (command.find_first_of(" \n") != string::npos) {
        params_only = _trim(command.substr(command.find_first_of(" \n")));
        first_param = params_only.substr(0, params_only.find_first_of(" \n"));
    }
    int job_id;
    try {
        job_id = stoi(first_param);
    } catch (std::exception &e) {
        bad_params = true;
    }
    bad_params = bad_params || (params_only.find_first_of(" \n") != string::npos);
    if (bad_params) {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    if (first_param == "") {
        jobs->getLastStoppedJob(&job_id);
        if (job_id == 0) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    JobsList::JobEntry *job = jobs->getJobById(job_id);
    if (!job) {
        cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        return;
    }
    //check if the job is stopped, send sigcont
    if (job->status != STOPPED) {
        cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
        return;
    }
    cout << job->cmd_line << " : " << job->pid << endl;
    DO_SYS(kill(job->pid, SIGCONT), kill);
    job->status = BACKGROUND;
}

//********** Quit Command ********************
void QuitCommand::execute() {
    string command = _trim(this->cmd_line);
    string params_only = "";
    string first_param = "";
    if (command.find_first_of(" \n") != string::npos) {
        params_only = _trim(command.substr(command.find_first_of(" \n")));
        first_param = params_only.substr(0, params_only.find_first_of(" \n"));
    }
    if (first_param == "kill") {
        jobs->killAllJobs();
    }
    exit(0);
}

//********** Kill Command ******************
void KillCommand::execute() {
    string command = _trim(cmd_line);
    string params_only, first_param;
    if (command.find_first_of(" \n") != string::npos) {
        params_only = _trim(command.substr(command.find_first_of(" \n")));
        first_param = params_only.substr(0, 1);
    }
    if (first_param != "-") {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    params_only = _trim(params_only.substr(1));
    string sig_str = params_only.substr(0, params_only.find_first_of(" \n"));

    int signal = 0;
    //check if signal is a valid number
    try {
        signal = stoi(sig_str);
    } catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    //check for legal signal
    if (signal < 1 || signal > 64) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if (params_only.find_first_of(" \n") == string::npos) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    params_only = _trim(params_only.substr(params_only.find_first_of(" \n")));
    string id_str = params_only.substr(0, params_only.find_first_of(" \n"));
    int job_id = 0;
    try {
        job_id = stoi(id_str);
    } catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    JobsList::JobEntry *job = jobs->getJobById(job_id);
    if (!job) {
        cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        return;
    }
    DO_SYS(kill(job->pid, signal), kill);
    if (signal == SIGSTOP) {
        job->status = STOPPED;
    }
    if (signal == SIGCONT) {
        job->status = BACKGROUND;
    }
    cout << "signal number " << signal << " was sent to pid " << job->pid << endl;
}

string removeTimeoutWord(string cmd_line , time_t* duration)
{
    cmd_line = _trim(cmd_line);
    if(cmd_line.find_first_of(" ") == string::npos)
    {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return "";
    }
    cmd_line = _trim(cmd_line.substr(cmd_line.find_first_of(" \n")));
    if(cmd_line.find_first_of(" ") == string::npos)
    {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return "";
    }
    string duration_str = cmd_line.substr(0 , cmd_line.find_first_of(" \n"));
    try{
        *duration = stoi(duration_str);
        if((*duration) < 0)
        {
            cerr << "smash error: timeout: invalid arguments" << endl;
            return "";
        }
    }catch (...){
        cerr << "smash error: timeout: invalid arguments" << endl;
        return "";
    }
    cmd_line = _trim(cmd_line.substr(cmd_line.find_first_of(" \n")));
    return cmd_line;
}

//********** External Command **************
void ExternalCommand::execute() {
    char command[COMMAND_ARGS_MAX_LENGTH];
    strcpy(command, cmd_line.c_str());
    bool is_bg = _isBackgroundComamnd(command);
    SmallShell &smash = SmallShell::getInstance();

    char *parsed[COMMAND_MAX_ARGS + 1];
    int num_of_args = _parseCommandLine(cmd_line.c_str(), parsed);


    bool is_timeout = _trim(parsed[0]) == "timeout";
    time_t duration;

    if(is_timeout)
    {
        string tmp = removeTimeoutWord(command , &duration);
        if(tmp != "")
        {
            strcpy(command , tmp.c_str());
        }
        else
        {
            return;
        }
    }

    if (is_bg) {
        _removeBackgroundSign(command);
        _parseCommandLine(command, parsed);
    }
    //TO DO: Timeout here
    int pid;
    DO_SYS(pid = fork(), fork);
    if (pid == -1) {
        return;
    }
    // son
    // if simple command
    if (pid == 0)
    {
        DO_SYS(setpgrp(), setpgrp);
        if (cmd_line.find('*') == string::npos && cmd_line.find('?') == string::npos)
        { // simple command
            DO_SYS(execvp(parsed[0], parsed), execvp);
        }
        else
        { // special command send to bash
            DO_SYS(execl("/bin/bash", "/bin/bash", "-c", command, nullptr), execl);
        }
    }
    else
    {
        if (is_bg)
        {
            smash.getJobsList()->addJob(this , pid , BACKGROUND);
        }
        else
        { // parent
            int status;
            smash.running_pid = pid;
            smash.running_cmd = cmd_line;
            DO_SYS(waitpid(pid, &status, WUNTRACED), waitpid);
            smash.running_pid = -1;
            smash.running_id = -1;
            smash.running_cmd = "";
        }
    }
    if(is_timeout)
    {
        smash.timeouts->addTimeoutCommand(pid , duration , cmd_line);
    }
}

//********** Pipe Command **************
void PipeCommand::execute()
{
    string redirect_cmd = cmd_line;
    redirect_cmd = _trim(redirect_cmd);
    string cmd1 = "";
    string cmd2 = "";
    bool is_pipe_err = redirect_cmd.find("|&") != string::npos;
    cmd1 = _trim(redirect_cmd.substr(0, redirect_cmd.find_first_of("|")));
    if (is_pipe_err)
    {
        cmd2 = redirect_cmd.substr(redirect_cmd.find_first_of("|&") + 2);
    }
    else
    {
        cmd2 = redirect_cmd.substr(redirect_cmd.find_first_of('|') + 1);
    }

    int pipe_arg[2], flag;
    DO_SYS(flag = pipe(pipe_arg), pipe);

    // pipe_arg[0] = read, pipe_arg[1] = write.
    int pid1, pid2;
    if(is_pipe_err)
    {
        DO_SYS(pid1 = fork() , fork);
        if(pid1 == 0)
        {
            DO_SYS(setpgrp() , setpgrp);
            DO_SYS(dup2(pipe_arg[1] , STDERR_FILENO) , dup2);
            DO_SYS(close(pipe_arg[0]) , close);
            DO_SYS(close(pipe_arg[1]) , close);
            SmallShell::getInstance().executeCommand(cmd1.c_str());
            exit(0);
        }
        DO_SYS(pid2 = fork() , fork);
        if(pid2 == 0)
        {
            DO_SYS(setpgrp() , setpgrp);
            DO_SYS(dup2(pipe_arg[0] , STDIN_FILENO) , dup2);
            DO_SYS(close(pipe_arg[0]) , close);
            DO_SYS(close(pipe_arg[1]) , close);
            SmallShell::getInstance().executeCommand(cmd2.c_str());
            exit(0);
        }
    }
    else
    {
        DO_SYS(pid1 = fork() , fork);
        if(pid1 == 0)
        {
            DO_SYS(setpgrp() , setpgrp);
            DO_SYS(dup2(pipe_arg[1] , STDOUT_FILENO) , dup2);
            DO_SYS(close(pipe_arg[0]) , close);
            DO_SYS(close(pipe_arg[1]) , close);
            SmallShell::getInstance().executeCommand(cmd1.c_str());
            exit(0);
        }
        DO_SYS(pid2 = fork() , fork);
        if(pid2 == 0)
        {
            DO_SYS(setpgrp() , setpgrp);
            DO_SYS(dup2(pipe_arg[0] , STDIN_FILENO) , dup2);
            DO_SYS(close(pipe_arg[0]) , close);
            DO_SYS(close(pipe_arg[1]) , close);
            SmallShell::getInstance().executeCommand(cmd2.c_str());
            exit(0);
        }
    }
    DO_SYS(close(pipe_arg[0]) , close);
    DO_SYS(close(pipe_arg[1]) , close);

    DO_SYS(waitpid(pid1 , NULL , 0) , waitpid);
    DO_SYS(waitpid(pid2 , NULL , 0) , waitpid);
}

// ************* Redirection Command***********
void RedirectionCommand::execute()
{
    string redirect_cmd = _trim(cmd_line);
    string cmd = "";
    string file = "";
    bool is_append_cmd = redirect_cmd.find("<<") != string::npos;
    cmd = _trim(redirect_cmd.substr(cmd.find_first_of('>')));
    int fd;

    int stdout_fd;
    DO_SYS(stdout_fd = dup(STDOUT_FILENO), dup);
    DO_SYS(close(STDOUT_FILENO), close);

    if (is_append_cmd)
    {
        // append command
        file = _trim(redirect_cmd.substr(redirect_cmd.find_first_of("<<") + 2));
        fd = open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);

    } else
    {
        // non append
        file = _trim(redirect_cmd.substr(redirect_cmd.find_first_of("<") + 1));
        fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
    }
    if (fd == -1)
    {
        perror("smash error: open failed");
        DO_SYS(dup2(stdout_fd, STDOUT_FILENO), dup2);
        DO_SYS(close(stdout_fd), close);
        return;
    }

    SmallShell::getInstance().executeCommand(cmd.c_str());

    DO_SYS(close(fd), close);
    DO_SYS(dup2(stdout_fd, STDOUT_FILENO), dup2);
    DO_SYS(close(stdout_fd), close);
}

// ***************TimeOut*******************
void TimeoutList::addTimeoutCommand(int pid, time_t duration, string cmd_line) {
    time_t now;
    if (time(&now) == ((time_t) - 1)) {
        perror("smash error: time failed");
        return;
    }

    int closest_alarm = alarm(0);
    // if no alarm is currently waiting or new alarm is closer
    if (closest_alarm == 0 || duration < closest_alarm)
    {
        alarm(duration);
    }
    else
    {
        alarm(closest_alarm);
    }
    shared_ptr<TimeoutEntry> new_entry = shared_ptr<TimeoutEntry>(new TimeoutEntry(pid, now, duration, cmd_line));
    commands.push_back(new_entry);
}

