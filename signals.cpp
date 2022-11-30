#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <vector>
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    int sys_result;
    //check if there is a running job
    if(smash.running_cmd != "" && smash.running_pid != -1)
    {
        DO_SYS(sys_result = kill(smash.running_pid , SIGSTOP) , kill);
        if(sys_result == -1)
        {
            return;
        }
        shared_ptr<ExternalCommand> to_stop = shared_ptr<ExternalCommand>(new ExternalCommand(smash.running_cmd.c_str()));
        smash.getJobsList()->addJob(to_stop.get() , true);
        cout << "smash: got ctrl-Z" << endl;
        cout << "smash: process " << smash.running_pid << " was stopped" << endl;
    }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    SmallShell& smash = SmallShell::getInstance();
    if(smash.running_pid == -1)
    {
        return;
    }
    int sys_result;
    DO_SYS(sys_result = kill(smash.running_pid , SIGKILL) , kill);
    if(sys_result == -1)
    {
        return;
    }
    cout << "smash: got ctrl-C" << endl;
    cout << "smash: process " << smash.running_pid << " was killed" << endl;;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    bool flag = true;
    SmallShell& smash = SmallShell::getInstance();
    while(flag)
    {
        cout << "smash: got an alaram" << endl;
        smash.getJobsList()->removeFinishedJobs();
        int seconds_to_alarm = 0;
        TimeoutList::TimeoutEntry* next_alarm = nullptr;
        // search for alarmed command
        for(int i = 0 ; i < smash.timeouts->commands.size() ; ++i)
        {
            if (!next_alarm)
            {
                next_alarm = smash.timeouts->commands[i].get();
                seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                continue;
            }
            if(smash.timeouts->commands[i]->duration + smash.timeouts->commands[i]->time_added < seconds_to_alarm)
            {
                next_alarm = smash.timeouts->commands[i].get();
                seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
            }
        }
        int pid = next_alarm->pid; //Segmentation
        int sys_result = waitpid(pid , NULL , WNOHANG);
        if(sys_result == 0)
        {
            DO_SYS(kill(pid , SIGKILL) , kill);
            cout << "smash: " << next_alarm->cmd_line << " timed out!" << endl;
        }
        // delete alarmed command
        for (int i = 0; i < smash.timeouts->commands.size(); ++i)
        {
            if (smash.timeouts->commands[i]->pid == next_alarm->pid) {
                smash.timeouts->commands.erase(smash.timeouts->commands.begin() + i);
            }
        }

        if (!smash.timeouts->commands.empty()) {
            next_alarm = nullptr;
            seconds_to_alarm = 0;
            // search for alarmed command
            for(int i = 0 ; i < smash.timeouts->commands.size() ; ++i)
            {
                if (!next_alarm)
                {
                    next_alarm = smash.timeouts->commands[i].get();
                    seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                    continue;
                }

                if(smash.timeouts->commands[i]->duration + smash.timeouts->commands[i]->time_added < seconds_to_alarm)
                {
                    next_alarm = smash.timeouts->commands[i].get();
                    seconds_to_alarm = next_alarm->duration + next_alarm->time_added;
                }
            }
            time_t now;
            if (time(&now) == ((time_t) -1))
            {
                perror("smash error: time faild");
                return;
            }

            seconds_to_alarm -= now;
        }
        alarm(seconds_to_alarm);
        flag = seconds_to_alarm == 0;
    }
}

