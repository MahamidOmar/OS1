#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

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
}

